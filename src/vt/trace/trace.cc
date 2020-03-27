/*
//@HEADER
// *****************************************************************************
//
//                                   trace.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/collective/collective_alg.h"
#include "vt/config.h"
#include "vt/scheduler/scheduler.h"
#include "vt/timing/timing.h"
#include "vt/trace/trace.h"
#include "vt/trace/trace_user.h"
#include "vt/utils/demangle/demangle.h"
#include "vt/trace/file_spec/spec.h"
#include "vt/objgroup/headers.h"
#include "vt/utils/memory/memory_usage.h"

#include <cinttypes>
#include <fstream>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
#include <stack>
#include <queue>

#include <mpi.h>
#include <zlib.h>

namespace vt { namespace trace {

using TraceContainersType = TraceContainers;

// Wrap zlib file implementation to allow header-clean declarations.
// Lifetime is same as the underlying stream.
struct vt_gzFile {
  gzFile file_type;
  vt_gzFile(gzFile pS) : file_type(pS) { }
};

using ArgType = vt::arguments::ArgConfig;

using LogType = Trace::LogType;

template <typename EventT>
struct TraceEventSeqCompare {
  bool operator()(EventT* const a, EventT* const b) const {
    return a->theEventSeq() < b->theEventSeq();
  }
};

Trace::Trace(std::string const& in_prog_name, std::string const& in_trace_name)
  : prog_name_(in_prog_name), trace_name_(in_trace_name),
    start_time_(getCurrentTime()), log_file_(nullptr)
{
  /*
   * Incremental flush mode for zlib. Several options are available:
   *
   *   Z_NO_FLUSH
   *   Z_PARTIAL_FLUSH
   *   Z_SYNC_FLUSH
   *   Z_FULL_FLUSH
   *   Z_FINISH
   *   Z_BLOCK
   *   Z_TREES
   *
   *  Turns out that any flush weaker than Z_FINISH, may not output a valid
   *  trace---some of these modes produce a completely valid gz file---but don't
   *  necessarily flush to the next gzprintf newline. Thus, during an exigent
   *  exit, the traces will not be readable by Projections, unless they are
   *  altered to clear out the last partially written line. Z_FINISH can be
   *  invoked for every incremental flush at the cost of space---compression
   *  across multiple flush epochs will be lost (see zlib docs).
   *
   *  For now, the incremental_flush_mode will be Z_SYNC_FINISH, implying that
   *  the gz files will have to cleaned if a segfault, etc. occurs. Change this
   *  to Z_FINISH if you want a clean flush.
   */

  incremental_flush_mode = Z_SYNC_FLUSH;
}

Trace::Trace() { }

/*static*/ void Trace::traceBeginIdleTrigger() {
  #if backend_check_enabled(trace_enabled)
    if (not theTrace()->inIdleEvent()) {
      theTrace()->beginIdle();
    }
  #endif
}

/*static*/ void Trace::traceEndIdleTrigger() {
  #if backend_check_enabled(trace_enabled)
    if (theTrace()->inIdleEvent()) {
      theTrace()->endIdle();
    }
  #endif
}

void Trace::initialize() {
  theSched()->registerTrigger(
    sched::SchedulerEvent::BeginIdle, traceBeginIdleTrigger
  );
  theSched()->registerTrigger(
    sched::SchedulerEvent::EndIdle, traceEndIdleTrigger
  );

  // Register a trace user event to demarcate flushes that occur
  flush_event_ = vt::trace::registerEventCollective("trace_flush");
}

void Trace::loadAndBroadcastSpec() {
  if (ArgType::vt_trace_spec) {
    auto spec_proxy = file_spec::TraceSpec::construct();
    theTerm()->produce();
    if (theContext()->getNode() == 0) {
      auto spec_ptr = spec_proxy.get();
      spec_ptr->parse();
      spec_ptr->broadcastSpec();
    }
    while (not spec_proxy.get()->specReceived()) {
      vt::runScheduler();
    }
    theTerm()->consume();
    spec_proxy_ = spec_proxy.getProxy();

    // Set enabled for the initial phase
    setTraceEnabledCurrentPhase(0);
  }
}

bool Trace::inIdleEvent() const {
  return idle_begun_;
}

void Trace::setupNames(
  std::string const& in_prog_name, std::string const& in_trace_name,
  std::string const& in_dir_name
) {
  auto const node = theContext()->getNode();

  prog_name_ = in_prog_name;
  trace_name_ = in_trace_name;
  start_time_ = getCurrentTime();

  std::string dir_name = (in_dir_name.empty()) ? prog_name_ + "_trace" : in_dir_name;

  char cur_dir[1024];
  if (getcwd(cur_dir, sizeof(cur_dir)) == nullptr) {
    vtAssert(false, "Must have current directory");
  }

  if (ArgType::vt_trace_dir.empty()) {
    full_dir_name_ = std::string(cur_dir) + "/" + dir_name;
  }
  else {
    full_dir_name_ = ArgType::vt_trace_dir;
  }

  if (full_dir_name_[full_dir_name_.size() - 1] != '/')
    full_dir_name_ = full_dir_name_ + "/";

  if (theContext()->getNode() == 0) {
    int flag = mkdir(full_dir_name_.c_str(), S_IRWXU);
    if ((flag < 0) && (errno != EEXIST)) {
      vtAssert(flag >= 0, "Must be able to make directory");
    }
  }

  auto const tc = util::demangle::DemanglerUtils::splitString(trace_name_, '/');
  auto const pc = util::demangle::DemanglerUtils::splitString(prog_name_, '/');
  auto const trace_name = tc[tc.size()-1];
  auto const prog_name = pc[pc.size()-1];

  auto const node_str = "." + std::to_string(node) + ".log.gz";
  if (ArgType::vt_trace_file.empty()) {
    full_trace_name_ = full_dir_name_ + trace_name;
    full_sts_name_   = full_dir_name_ + prog_name + ".sts";
  } else {
    full_trace_name_ = full_dir_name_ + ArgType::vt_trace_file + node_str;
    full_sts_name_   = full_dir_name_ + ArgType::vt_trace_file + ".sts";
  }
}

/*virtual*/ Trace::~Trace() {
  // Not good - not much to do in late destruction.
  vtWarnIf(
    not open_events_.empty(),
    "Trying to dump traces with open events?"
  );

  cleanupTracesFile();
}

void Trace::addUserNote(std::string const& note) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node,
    "Trace::addUserNote: note={}\n",
    note
  );

  auto const type = TraceConstantsType::UserSuppliedNote;
  auto const time = getCurrentTime();

  logEvent(
    LogType{time, type, note, Log::UserDataType{}}
  );
}

void Trace::addUserData(int32_t data) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node,
    "Trace::addUserData: data={}\n",
    data
  );

  auto const type = TraceConstantsType::UserSupplied;
  auto const time = getCurrentTime();

  logEvent(
    LogType{time, type, std::string{}, data}
  );
}

void Trace::addUserBracketedNote(
  double const begin, double const end, std::string const& note,
  TraceEventIDType const event
) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node,
    "Trace::addUserBracketedNote: begin={}, end={}, note={}, event={}\n",
    begin, end, note, event
  );

  auto const type = TraceConstantsType::UserSuppliedBracketedNote;

  logEvent(
    LogType{begin, end, type, note, event}
  );
}

UserEventIDType Trace::registerUserEventColl(std::string const& name) {
  return user_event_.collective(name);
}

UserEventIDType Trace::registerUserEventRoot(std::string const& name) {
  return user_event_.rooted(name);
}

UserEventIDType Trace::registerUserEventHash(std::string const& name) {
  return user_event_.hash(name);
}

void Trace::registerUserEventManual(
  std::string const& name, UserSpecEventIDType id
) {
  user_event_.user(name, id);
}

void insertNewUserEvent(UserEventIDType event, std::string const& name) {
  #if backend_check_enabled(trace_enabled)
    theTrace()->user_event_.insertEvent(event, name);
  #endif
}

void Trace::addUserEvent(UserEventIDType event) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node,
    "Trace::addUserEvent: event={:x}\n",
    event
  );

  auto const type = TraceConstantsType::UserEvent;
  auto const time = getCurrentTime();
  NodeType const node = theContext()->getNode();

  logEvent(
    LogType{time, type, node, event, true}
  );
}

void Trace::addUserEventManual(UserSpecEventIDType event) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node,
    "Trace::addUserEventManual: event={:x}\n",
    event
  );

  auto id = user_event_.createEvent(true, false, 0, event);
  addUserEvent(id);
}

void Trace::addUserEventBracketed(UserEventIDType event, double begin, double end) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node,
    "Trace::addUserEventBracketed: event={:x}, begin={}, end={}\n",
    event, begin, end
  );

  auto const type = TraceConstantsType::UserEventPair;
  NodeType const node = theContext()->getNode();

  logEvent(
    LogType{begin, type, node, event, true}
  );
  logEvent(
    LogType{end, type, node, event, false}
  );
}

void Trace::addUserEventBracketedBegin(UserEventIDType event) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node,
    "Trace::addUserEventBracketedBegin: event={:x}\n",
    event
  );

  auto const type = TraceConstantsType::BeginUserEventPair;
  auto const time = getCurrentTime();
  NodeType const node = theContext()->getNode();

  logEvent(
    LogType{time, type, node, event, true}
  );
}

void Trace::addUserEventBracketedEnd(UserEventIDType event) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node,
    "Trace::addUserEventBracketedEnd: event={:x}\n",
    event
  );

  auto const type = TraceConstantsType::EndUserEventPair;
  auto const time = getCurrentTime();
  NodeType const node = theContext()->getNode();

  logEvent(
    LogType{time, type, node, event, true}
  );
}

void Trace::addUserEventBracketedManualBegin(UserSpecEventIDType event) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  auto id = user_event_.createEvent(true, false, 0, event);
  addUserEventBracketedBegin(id);
}

void Trace::addUserEventBracketedManualEnd(UserSpecEventIDType event) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  auto id = user_event_.createEvent(true, false, 0, event);
  addUserEventBracketedEnd(id);
}

void Trace::addUserEventBracketedManual(
  UserSpecEventIDType event, double begin, double end
) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node,
    "Trace::addUserEventBracketedManual: event={:x}, begin={}, end={}\n",
    event, begin, end
  );

  auto id = user_event_.createEvent(true, false, 0, event);
  addUserEventBracketed(id, begin, end);
}

TraceProcessingTag Trace::beginProcessing(
  TraceEntryIDType const ep, TraceMsgLenType const len,
  TraceEventIDType const event, NodeType const from_node, double const time,
  uint64_t const idx1, uint64_t const idx2, uint64_t const idx3,
  uint64_t const idx4
) {
  if (not checkDynamicRuntimeEnabled()) {
    return TraceProcessingTag{};
  }

  debug_print(
    trace, node,
    "event_start: ep={}, event={}, time={}, from={}\n",
    ep, event, time, from_node
  );

  auto const type = TraceConstantsType::BeginProcessing;

  TraceEventIDType loggedEvent = logEvent(
    LogType{
      time, ep, type, event, len, from_node, idx1, idx2, idx3, idx4
    }
  );

  if (ArgType::vt_trace_memory_usage) {
    auto usage = util::memory::MemoryUsage::get();
    addMemoryEvent(usage->getFirstUsage());
  }

  return TraceProcessingTag{ep, loggedEvent};
}

void Trace::endProcessing(
  TraceProcessingTag const& processing_tag,
  double const time
) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  TraceEntryIDType ep = processing_tag.ep_;
  TraceEventIDType event = processing_tag.event_;

  // Allow no-op cases (ie. beginProcessing disabled)
  if (ep == trace::no_trace_entry_id) {
    return;
  }

  vtAssert(
    not open_events_.empty()
    // This is current contract expectations; however it precludes async closing.
    and open_events_.top().ep == ep
    and open_events_.top().event == event,
    "Event being closed must be on the top of the open event stack."
  );

  // Final event is same as original with a few .. tweaks.
  // Always done PRIOR TO restarts.
  traces_.push(
    LogType{open_events_.top(), time, TraceConstantsType::EndProcessing}
  );
  open_events_.pop();

  if (not open_events_.empty()) {
    // Emit a '[re]start' event for the reactivated stack item.
    traces_.push(
      LogType{open_events_.top(), time, TraceConstantsType::BeginProcessing}
    );
  }

  // Unlike logEvent there is currently no flush here.
}

void Trace::endProcessing(
  TraceEntryIDType const ep, TraceMsgLenType const len,
  TraceEventIDType const event, NodeType const from_node, double const time,
  uint64_t const idx1, uint64_t const idx2, uint64_t const idx3,
  uint64_t const idx4
) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  if (ArgType::vt_trace_memory_usage) {
    auto usage = util::memory::MemoryUsage::get();
    addMemoryEvent(usage->getFirstUsage());
  }

  debug_print(
    trace, node,
    "event_stop: ep={}, event={}, time={}, from_node={}\n",
    ep, event, time, from_node
  );

  auto const type = TraceConstantsType::EndProcessing;

  logEvent(
    LogType{
      time, ep, type, event, len, from_node, idx1, idx2, idx3, idx4
    }
  );
}

void Trace::addMemoryEvent(std::size_t memory, double time) {
  auto const type = TraceConstantsType::MemoryUsageCurrent;
  logEvent(LogType{time, type, memory});
}

void Trace::beginIdle(double const time) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node, "begin_idle: time={}\n", time
  );

  auto const type = TraceConstantsType::BeginIdle;
  NodeType const node = theContext()->getNode();

  logEvent(
    LogType{time, type, node}
  );
  idle_begun_ = true; // must set AFTER logEvent
}

void Trace::endIdle(double const time) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  debug_print(
    trace, node, "end_idle: time={}\n", time
  );

  auto const type = TraceConstantsType::EndIdle;
  NodeType const node = theContext()->getNode();

  logEvent(
    LogType{time, type, node}
  );
  idle_begun_ = false; // must set AFTER logEvent
}

TraceEventIDType Trace::messageCreation(
  TraceEntryIDType const ep, TraceMsgLenType const len, double const time
) {
  if (not checkDynamicRuntimeEnabled()) {
    return no_trace_event;
  }

  auto const type = TraceConstantsType::Creation;
  NodeType const node = theContext()->getNode();

  return logEvent(
    LogType{time, ep, type, node, len}
  );
}

TraceEventIDType Trace::messageCreationBcast(
  TraceEntryIDType const ep, TraceMsgLenType const len, double const time
) {
  if (not checkDynamicRuntimeEnabled()) {
    return no_trace_event;
  }

  auto const type = TraceConstantsType::CreationBcast;
  NodeType const node = theContext()->getNode();

  return logEvent(
    LogType{time, ep, type, node, len}
  );
}

TraceEventIDType Trace::messageRecv(
  TraceEntryIDType const ep, TraceMsgLenType const len,
  NodeType const from_node, double const time
) {
  if (not checkDynamicRuntimeEnabled()) {
    return no_trace_event;
  }

  auto const type = TraceConstantsType::MessageRecv;
  NodeType const node = theContext()->getNode();

  return logEvent(
    LogType{time, ep, type, node, len}
  );
}

bool Trace::checkDynamicRuntimeEnabled() {
  /*
   * enabled_ -> this is the dynamic check that can be disabled at any point via
   * the application
   *
   * checkEnabled() -> this is the "static" runtime check, may be disabled for a
   * subset of processors when trace mod is used to reduce overhead
   *
   * trace_enabled_cur_phase_ -> this is whether tracing is enabled for the
   * current phase (LB phase), which can be disabled via a trace enable
   * specification file
   */
  return
    enabled_ and
    traceWritingEnabled(theContext()->getNode()) and
    trace_enabled_cur_phase_;
}

void Trace::setTraceEnabledCurrentPhase(PhaseType cur_phase) {
  if (spec_proxy_ != vt::no_obj_group) {
    // SpecIndex is signed due to negative/positive, phase is not signed
    auto spec_index = static_cast<file_spec::TraceSpec::SpecIndex>(cur_phase);
    vt::objgroup::proxy::Proxy<file_spec::TraceSpec> proxy(spec_proxy_);
    bool ret = proxy.get()->checkTraceEnabled(spec_index);

    if (trace_enabled_cur_phase_ != ret) {
      auto time = getCurrentTime();
      // Close and pop everything, we are disabling traces at this point
      while (not open_events_.empty()) {
        traces_.push(
          LogType{open_events_.top(), time, TraceConstantsType::EndProcessing}
        );
        open_events_.pop();
      }

      // Go ahead and perform a trace flush when tracing is disabled (and was
      // previously enabled) to reduce memory footprint.
      if (not ret and ArgType::vt_trace_flush_size != 0) {
        writeTracesFile(incremental_flush_mode, true);
      }
    }

    trace_enabled_cur_phase_ = ret;

    debug_print(
      gen, node,
      "setTraceEnabledCurrentPhase: phase={}, enabled={}\n",
      cur_phase,
      trace_enabled_cur_phase_
    );
  }
}

TraceEventIDType Trace::logEvent(LogType&& log) {
  if (not checkDynamicRuntimeEnabled()) {
    return no_trace_event;
  }

  vtAssert(
   log.ep == no_trace_entry_id
   or TraceRegistry::getEvent(log.ep).theEventId() not_eq no_trace_entry_id,
    "Event must exist that was logged"
  );

  // close any idle event as soon as we encounter any other type of event
  if (idle_begun_ and
      log.type != TraceConstantsType::BeginIdle and
      log.type != TraceConstantsType::EndIdle) {
    endIdle();
  }

  switch (log.type) {
  case TraceConstantsType::BeginProcessing: {

    if (not open_events_.empty()) {
      // Emit a 'stop' event for the current stack item;
      // another '[re]start' event will be emitted on group end.
      double logTime = log.time;
      traces_.push(
        LogType{open_events_.top(), logTime, TraceConstantsType::EndProcessing}
      );
    }

    open_events_.push(log /* copy, not forwarding rv-ref */);

    break;
  }
  case TraceConstantsType::EndProcessing: {
    // Ideal route is to call endProcessing(tag) directly as all
    // data should be on the beginProcessing event and not changed after.
    // This case remains for compatibility.

    vtAssert(
      open_events_.top().ep == log.ep and
      open_events_.top().type == TraceConstantsType::BeginProcessing,
      "Top event should be correct type and event"
    );

    // Steal top event information
    LogType const& top = open_events_.top();
    TraceProcessingTag processing_tag{top.ep, top.event};
    endProcessing(processing_tag);

    return trace::no_trace_event;
  }
  case TraceConstantsType::Creation:
  case TraceConstantsType::CreationBcast:
  case TraceConstantsType::MessageRecv:
    log.event = cur_event_++;
    break;
  case TraceConstantsType::BeginIdle:
  case TraceConstantsType::EndIdle:
    log.event = no_trace_event;
    break;
  case TraceConstantsType::MemoryUsageCurrent:
  case TraceConstantsType::UserSupplied:
  case TraceConstantsType::UserSuppliedNote:
  case TraceConstantsType::UserSuppliedBracketedNote:
    // Accept log->event as is.
    break;
  case TraceConstantsType::UserEvent:
  case TraceConstantsType::UserEventPair:
    log.event = cur_event_;
    if (not log.user_data().user_start) {
      cur_event_++;
    }
    break;
  case TraceConstantsType::BeginUserEventPair:
  case TraceConstantsType::EndUserEventPair:
    log.event = cur_event_++;
    break;
  default:
    vtAssert(0, "Not implemented");
    return 0;
  }

  // Normal case of event emitted at end
  TraceEventIDType event = log.event;
  traces_.push(std::move(log));

  return event;
}

/*static*/ bool Trace::traceWritingEnabled(NodeType node) {
  return (ArgType::vt_trace
          and (ArgType::vt_trace_mod == 0
               or (node % ArgType::vt_trace_mod == 0)));
}

/*static*/ bool Trace::isStsOutputNode(NodeType node) {
  return (ArgType::vt_trace
          and node == designated_root_node);
}

void Trace::enableTracing() {
  enabled_ = true;
}

void Trace::disableTracing() {
  enabled_ = false;
}

void Trace::cleanupTracesFile() {
  auto const& node = theContext()->getNode();
  if (not (traceWritingEnabled(node)
           or isStsOutputNode(node))) {
    return;
  }

  // No more events can be written.
  disableTracing();

  //--- Dump everything into an output file and close.
  writeTracesFile(Z_FINISH, false);

  assert(log_file_ && "Trace file must be open"); // opened in writeTracesFile
  outputFooter(log_file_.get(), node, start_time_);
  gzclose(log_file_.get()->file_type);
  log_file_ = nullptr;
}

void Trace::flushTracesFile(bool useGlobalSync) {
  if (ArgType::vt_trace_flush_size == 0) {
    // Flush the traces at the end only
    return;
  }
  if (useGlobalSync) {
    // Synchronize all the nodes before flushing the traces
    // (Consider pushing out: barrier usages are probably domain-specific.)
    theCollective()->barrier();
  }
  if (traces_.size() >= static_cast<std::size_t>(ArgType::vt_trace_flush_size)) {
    writeTracesFile(incremental_flush_mode, true);
  }
}

void Trace::writeTracesFile(int flush, bool is_incremental_flush) {
  auto const node = theContext()->getNode();

  size_t to_write = traces_.size();

  if (traceWritingEnabled(node) and to_write > 0) {
    if (not log_file_) {
      auto path = full_trace_name_;
      log_file_ = std::make_unique<vt_gzFile>(gzopen(path.c_str(), "wb"));
      outputHeader(log_file_.get(), node, start_time_);
    }

    debug_print(
      trace, node,
      "write_traces_file: to_write={}, already_written={}, "
      "event_parents_types={}, event_types={}\n",
      traces_.size(), trace_write_count_,
      TraceContainersType::getEventTypeContainer()->size(),
      TraceContainersType::getEventContainer()->size()
    );

    if (theContext()->getNode() == 0) {
      vt_print(
        trace,
        "writeTracesFile: to_write={}, already_written={}\n",
        traces_.size(), trace_write_count_
      );
    }

    vt::trace::TraceScopedEvent scope(
      is_incremental_flush ? flush_event_ : no_user_event_id
    );
    outputTraces(
      log_file_.get(), traces_, start_time_, flush
    );

    trace_write_count_ += to_write;
  }

  if (not isStsOutputNode(node)) {
    return;
  }

  if ((flush == Z_FINISH) or (not wrote_sts_file_)) {
    std::ofstream file;
    auto name = full_sts_name_;
    file.open(name);
    outputControlFile(file);
    file.close();
    wrote_sts_file_ = true;
  }
}

/*static*/ void Trace::outputTraces(
  vt_gzFile* file, TraceContainerType& traces,
  double start_time, int flush
) {
  auto const num_nodes = theContext()->getNumNodes();
  gzFile gzfile = file->file_type;

  while (not traces.empty()) {
    LogType const& log = traces.front();

    auto const& converted_time = timeToInt(log.time - start_time);
    auto const type = static_cast<
      std::underlying_type<decltype(log.type)>::type
    >(log.type);

    vtAssert(
      log.ep == no_trace_entry_id
      or TraceRegistry::getEvent(log.ep).theEventId() not_eq no_trace_entry_id,
      "Event must exist that was logged"
    );

    // Widen to unsigned long for %lu format
    unsigned long event_seq_id = log.ep == no_trace_entry_id
      ? 0 // no_trace_entry_seq != 0 (perhaps shift offsets..).
      : TraceRegistry::getEvent(log.ep).theEventSeq();

    switch (log.type) {
    case TraceConstantsType::BeginProcessing: {
      auto const& sdata = log.sys_data();
      gzprintf(
        gzfile,
        "%d %d %lu %lld %d %d %zu 0 %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log.event,
        log.node,
        sdata.msg_len,
        sdata.idx1,
        sdata.idx2,
        sdata.idx3,
        sdata.idx4
      );
      break;
    }
    case TraceConstantsType::EndProcessing: {
      auto const& sdata = log.sys_data();
      gzprintf(
        gzfile,
        "%d %d %lu %lld %d %d %zu 0 %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log.event,
        // Future: remove data from EndProcessing (accept only in begin)
        log.node,
        sdata.msg_len,
        sdata.idx1,
        sdata.idx2,
        sdata.idx3,
        sdata.idx4
      );
      break;
    }
    case TraceConstantsType::BeginIdle:
      gzprintf(
        gzfile,
        "%d %lld %d\n",
        type,
        converted_time,
        // Future: remove node from idle begin/end (always 'this' node!)
        log.node
      );
      break;
    case TraceConstantsType::EndIdle:
      gzprintf(
        gzfile,
        "%d %lld %d\n",
        type,
        converted_time,
        // Future: remove node from idle begin/end (always 'this' node!)
        log.node
      );
      break;
    case TraceConstantsType::CreationBcast: {
      auto const& sdata = log.sys_data();
      gzprintf(
        gzfile,
        "%d %d %lu %lld %d %d %zu 0 %d\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log.event,
        log.node,
        sdata.msg_len,
        num_nodes
      );
      break;
    }
    case TraceConstantsType::Creation: {
      auto const& sdata = log.sys_data();
      gzprintf(
        gzfile,
        "%d %d %lu %lld %d %d %zu 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log.event,
        log.node,
        sdata.msg_len
      );
      break;
    }
    case TraceConstantsType::UserEvent:
    case TraceConstantsType::UserEventPair:
    case TraceConstantsType::BeginUserEventPair:
    case TraceConstantsType::EndUserEventPair: {
      auto const& udata = log.user_data();
      gzprintf(
        gzfile,
        "%d %lld %lld %d %d %d\n",
        type,
        udata.user_event,
        converted_time,
        log.event,
        log.node,
        0
      );
      break;
    }
    case TraceConstantsType::UserSupplied: {
      auto const& udata = log.user_data();
      gzprintf(
        gzfile,
        "%d %d %lld\n",
        type,
        udata.user_data,
        converted_time
      );
      break;
    }
    case TraceConstantsType::UserSuppliedNote: {
      auto const& udata = log.user_data();
      gzprintf(
        gzfile,
        "%d %lld %zu %s\n",
        type,
        converted_time,
        udata.user_note.length(),
        udata.user_note.c_str()
      );
      break;
    }
    case TraceConstantsType::UserSuppliedBracketedNote: {
      auto const& udata = log.user_data();
      auto const converted_end_time = timeToInt(log.end_time - start_time);
      gzprintf(
        gzfile,
        "%d %lld %lld %d %zu %s\n",
        type,
        converted_time,
        converted_end_time,
        log.event,
        udata.user_note.length(),
        udata.user_note.c_str()
      );
      break;
    }
    case TraceConstantsType::MemoryUsageCurrent: {
      auto const& sdata = log.sys_data();
      gzprintf(
        gzfile,
        "%d %zu %lld \n",
        type,
        sdata.msg_len,
        converted_time
      );
      break;
    }
    default:
      auto log_type = static_cast<std::underlying_type<TraceConstantsType>::type>(log.type);
      vtAssertInfo(false,
        "Unimplemented log type", converted_time, log.node, log_type
      );
    }

    // Poof!
    traces.pop();
  }

  // Actually call flush to get it written to disk
  gzflush(gzfile, flush);
}

void Trace::outputControlFile(std::ofstream& file) {

  using ContainerEventSortedType = std::map<
    TraceContainerEventType::mapped_type*, bool, TraceEventSeqCompare<TraceEventType>
  >;

  using ContainerEventTypeSortedType = std::map<
    TraceContainerEventClassType::mapped_type*, bool, TraceEventSeqCompare<EventClassType>
  >;

  auto const num_nodes = theContext()->getNumNodes();

  auto* event_types = TraceContainersType::getEventTypeContainer();
  auto* events = TraceContainersType::getEventContainer();

  auto const num_event_types = event_types->size();
  auto const num_events = events->size();
  auto const num_user_events = user_event_.getEvents().size();

  file << "PROJECTIONS_ID\n"
       << "VERSION 7.0\n"
       << "TOTAL_PHASES 1\n"
       << "MACHINE vt\n"
       << "PROCESSORS " << num_nodes << "\n"
       << "TOTAL_CHARES " << num_event_types << "\n"
       << "TOTAL_EPS " << num_events << "\n"
       << "TOTAL_MSGS 1\n"
       << "TOTAL_PSEUDOS 0\n"
       << "TOTAL_EVENTS " << num_user_events
       << std::endl;

  ContainerEventSortedType sorted_event;
  ContainerEventTypeSortedType sorted_event_type;

  for (auto&& elem : *TraceContainersType::getEventContainer()) {
    sorted_event.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(&elem.second),
      std::forward_as_tuple(true)
    );
  }

  for (auto&& elem : *TraceContainersType::getEventTypeContainer()) {
    sorted_event_type.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(&elem.second),
      std::forward_as_tuple(true)
    );
  }

  for (auto&& event : sorted_event_type) {
    auto const name = event.first->theEventName();
    auto const id = event.first->theEventSeq();

    auto const out_name = std::string("::" + name);

    file << "CHARE "
         << id << " "
         << out_name << " "
         << std::endl;
  }

  for (auto&& event : sorted_event) {
    auto const name = event.first->theEventName();
    auto const type = event.first->theEventTypeSeq();
    auto const id = event.first->theEventSeq();

    file << "ENTRY CHARE "
         << id << " "
         << name << " "
         << type << " "
         << 0 << " "
         << std::endl;
  }

  file << "MESSAGE 0 0\n"
       << "TOTAL_STATS 0\n";

  for (auto&& elm : user_event_.getEvents()) {
    auto const id = elm.first;
    auto const name = elm.second;

    file << "EVENT "
         << id << " "
         << name << " "
         << std::endl;
  }

  file << "TOTAL_FUNCTIONS 0\n"
       << "END\n"
       << std::endl;
}

/*static*/ void Trace::outputHeader(
  vt_gzFile* file, NodeType const node, double const start
) {
  gzFile gzfile = file->file_type;
  // Output header for projections file
  // '6' means COMPUTATION_BEGIN to Projections: this starts a trace
  gzprintf(gzfile, "PROJECTIONS-RECORD 0\n");
  gzprintf(gzfile, "6 0\n");
}

/*static*/ void Trace::outputFooter(
  vt_gzFile* file, NodeType const node, double const start
) {
  gzFile gzfile = file->file_type;
  // Output footer for projections file,
  // '7' means COMPUTATION_END to Projections
  gzprintf(gzfile, "7 %lld\n", timeToInt(getCurrentTime() - start));
}

}} //end namespace vt::trace
