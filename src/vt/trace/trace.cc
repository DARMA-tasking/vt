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
#include "vt/utils/demangle/demangle.h"

#include <cinttypes>
#include <fstream>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>

#include <mpi.h>
#include <zlib.h>


namespace vt { namespace trace {

struct vt_gzFile {
  gzFile file_type;
  //---
  vt_gzFile(gzFile_s *pS) : file_type(pS) { }
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
{ }

Trace::Trace() { }

/*static*/ void Trace::traceBeginIdleTrigger() {
  #if backend_check_enabled(trace_enabled)
    if (not theTrace()->inIdleEvent()) {
      theTrace()->beginIdle();
    }
  #endif
}

void Trace::initialize() {
  if (checkDynamicRuntimeEnabled()) {
    traces_.reserve(trace_reserve_count);
  }

  theSched()->registerTrigger(
    sched::SchedulerEvent::BeginIdle, traceBeginIdleTrigger
  );
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
    std::make_unique<LogType>(time, type, note, Log::UserDataType{})
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
    std::make_unique<LogType>(time, type, std::string{}, data)
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
    std::make_unique<LogType>(begin, end, type, note, event)
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
    std::make_unique<LogType>(time, type, node, event, true)
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
    std::make_unique<LogType>(begin, type, node, event, true)
  );
  logEvent(
    std::make_unique<LogType>(end, type, node, event, false)
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
    std::make_unique<LogType>(time, type, node, event, true)
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
    std::make_unique<LogType>(time, type, node, event, false)
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

void Trace::beginProcessing(
  TraceEntryIDType const ep, TraceMsgLenType const len,
  TraceEventIDType const event, NodeType const from_node, double const time,
  uint64_t const idx1, uint64_t const idx2, uint64_t const idx3,
  uint64_t const idx4
) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  auto const type = TraceConstantsType::BeginProcessing;
  LogPtrType log = new LogType(time, ep, type);

  debug_print(
    trace, node,
    "event_start: ep={}, event={}, time={}, from={}\n",
    ep, event, time, from_node
  );

  auto const type = TraceConstantsType::BeginProcessing;

  logEvent(
    std::make_unique<LogType>(
      time, ep, type, event, len, from_node, idx1, idx2, idx3, idx4
    )
  );
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

  debug_print(
    trace, node,
    "event_stop: ep={}, event={}, time={}, from_node={}\n",
    ep, event, time, from_node
  );

  auto const type = TraceConstantsType::EndProcessing;

  logEvent(
    std::make_unique<LogType>(
      time, ep, type, event, len, from_node, idx1, idx2, idx3, idx4
    )
  );

  if (open_events_.empty()) {
    cur_stop_ = traces_.size();
  }
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
    std::make_unique<LogType>(time, type, node)
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
    std::make_unique<LogType>(time, type, node)
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
    std::make_unique<LogType>(time, ep, type, node, len)
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
    std::make_unique<LogType>(time, ep, type, node, len)
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
    std::make_unique<LogType>(time, ep, type, node, len)
  );
}

bool Trace::checkDynamicRuntimeEnabled() {
  /*
   * enabled_ -> this is the dynamic check that can be disabled at any point via
   * the application
   *
   * checkEnabled() -> this is the "static" runtime check, may be disabled for a
   * subset of processors when trace mod is used to reduce overhead
   */
  return enabled_ and checkEnabled();
}

void Trace::editLastEntry(std::function<void(LogPtrType)> fn) {
  if (not enabled_ || not traceWritingEnabled(theContext()->getNode())) {
    return;
  }
  if (traces_.empty()) {
    return;
  }
  //---
  auto const trace_cont_size = traces_.size();
  fn(traces_.at(trace_cont_size - 1));
}

TraceEventIDType Trace::logEvent(std::unique_ptr<LogType> log) {
  assert(log != nullptr && "log cannot be null");

  if (not enabled_ || not traceWritingEnabled(theContext()->getNode())) {
    return no_trace_event;
  }

  vtAssert(
   log->ep == no_trace_entry_id
   or TraceRegistry::getEvent(log->ep).theEventId() not_eq no_trace_entry_id,
    "Event must exist that was logged"
  );

  // close any idle event as soon as we encounter any other type of event
  if (idle_begun_ and
      log->type != TraceConstantsType::BeginIdle and
      log->type != TraceConstantsType::EndIdle) {
    endIdle();
  }

  auto grouped_begin = [&]() -> TraceEventIDType {
    TraceEventIDType event = log->event;
    double logTime = log->time;

    if (not open_events_.empty()) {
      LogType* top_event = open_events_.top();
      auto end_log = std::make_unique<LogType>(
        logTime,
        top_event->ep,
        TraceConstantsType::EndProcessing,
        top_event->event,
        top_event->msg_len,
        top_event->node,
        top_event->idx1,
        top_event->idx2,
        top_event->idx3,
        top_event->idx4
      );
      traces_.push_back(std::move(end_log));
    }

    // push on open stack -- object is owned by trace collection, NOT stack.
    open_events_.push(log.get());

    traces_.push_back(std::move(log));

    return event;
  };

  auto grouped_end = [&]() -> TraceEventIDType {
    TraceEventIDType event = log->event;
    double logTime = log->time;

    vtAssert(
      not open_events_.empty(), "Stack should be empty"
    );

    vtAssert(
      open_events_.top()->ep == log->ep and
      open_events_.top()->type == TraceConstantsType::BeginProcessing,
      "Top event should be correct type and event"
    );

    // match event with the one that this ends
    log->event = open_events_.top()->event;

    // set up begin/end links
    open_events_.pop();

    traces_.push_back(std::move(log));

    if (not open_events_.empty()) {
      LogType* top_event = open_events_.top();
      auto begin_log = std::make_unique<LogType>(
        logTime,
        top_event->ep,
        TraceConstantsType::BeginProcessing,
        top_event->event,
        top_event->msg_len,
        top_event->node,
        top_event->idx1,
        top_event->idx2,
        top_event->idx3,
        top_event->idx4
      );
      traces_.push_back(std::move(begin_log));
    }

    return event;
  };

  auto basic_new_event_create = [&]() -> TraceEventIDType {
    TraceEventIDType event = cur_event_++;
    log->event = event;
    traces_.push_back(std::move(log));

    return event;
  };

  auto basic_no_event_create = [&]() -> TraceEventIDType {
    TraceEventIDType event = no_trace_event;
    log->event = event;
    traces_.push_back(std::move(log));

    return event;
  };

  auto basic_cur_event = [&]() -> TraceEventIDType {
    TraceEventIDType event = cur_event_;
    log->event = event;
    traces_.push_back(std::move(log));

    return event;
  };

  auto basic_create = [&]() -> TraceEventIDType {
    TraceEventIDType event = log->event;
    traces_.push_back(std::move(log));

    return event;
  };

  switch (log->type) {
  case TraceConstantsType::BeginProcessing:
    return grouped_begin();
  case TraceConstantsType::EndProcessing:
    return grouped_end();
  case TraceConstantsType::Creation:
  case TraceConstantsType::CreationBcast:
  case TraceConstantsType::MessageRecv:
    return basic_new_event_create();
  case TraceConstantsType::BeginIdle:
  case TraceConstantsType::EndIdle:
    return basic_no_event_create();
  case TraceConstantsType::UserSupplied:
  case TraceConstantsType::UserSuppliedNote:
  case TraceConstantsType::UserSuppliedBracketedNote:
    return basic_create();
  case TraceConstantsType::UserEvent:
  case TraceConstantsType::UserEventPair:
    return log->user_start ? basic_cur_event() : basic_new_event_create();
    break;
  case TraceConstantsType::BeginUserEventPair:
  case TraceConstantsType::EndUserEventPair:
    return basic_new_event_create();
  default:
    vtAssert(0, "Not implemented");
    return 0;
  }
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
  //--- Dump everything into an output file
  cur_stop_ = traces_.size();
  writeTracesFile(Z_FINISH);
  outputFooter(node, start_time_, log_file_.get());
  gzclose(log_file_->file_type);
}

void Trace::flushTracesFile(bool useGlobalSync) {
  if (ArgType::vt_trace_flush_size == 0) {
    // Flush the traces at the end only
    return;
  }
  if (useGlobalSync) {
    // Synchronize all the nodes before flushing the traces
    theCollective()->barrier();
  }
  if (traces_.size() > cur_ + ArgType::vt_trace_flush_size) {
    writeTracesFile(Z_SYNC_FLUSH);
  }
}

void Trace::writeTracesFile(int flush) {
  auto const node = theContext()->getNode();

  debug_print(
    trace, node,
    "write_traces_file: traces.size={}, "
    "event_type_container.size={}, event_container.size={}\n",
    traces_.size(),
    TraceContainersType::getEventTypeContainer()->size(),
    TraceContainersType::getEventContainer()->size()
  );

  if (traceWritingEnabled(node)) {
    auto path = full_trace_name_;
    if (not file_is_open_) {
      log_file_ = std::make_unique<vt_gzFile>(gzopen(path.c_str(), "wb"));
      outputHeader(node, start_time_, log_file_.get());
      file_is_open_ = true;
    }
    writeLogFile(log_file_.get(), traces_);
    gzflush(log_file_->file_type, flush);
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

void Trace::writeLogFile(vt_gzFile *file_, TraceContainerType &traces) {
  auto const num_nodes = theContext()->getNumNodes();
  size_t stop_point = cur_stop_;

  for (size_t i = cur_; i < stop_point; i++) {
    auto& log = traces[i];
    auto const& converted_time = timeToInt(log->time - start_time_);
    auto const type = static_cast<
      std::underlying_type<decltype(log->type)>::type
    >(log->type);

    auto event_iter = TraceContainersType::getEventContainer()->find(log->ep);
    auto file = file_->file_type;

    if ((log->ep != no_trace_entry_id) and
      (event_iter == TraceContainersType::getEventContainer()->end())) {
      vtAssert(false, "Event must exist that was logged");
    }

    TraceEntryIDType event_seq_id;
    if (event_iter == TraceContainersType::getEventContainer()->end()) {
      event_seq_id = 0;
    } else {
      event_seq_id = log->ep == no_trace_entry_id ?
        no_trace_entry_id : event_iter->second.theEventSeq();
    }

    switch (log->type) {
    case TraceConstantsType::BeginProcessing:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d %d 0 %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node,
        log->msg_len,
        log->idx1,
        log->idx2,
        log->idx3,
        log->idx4
      );
      break;
    case TraceConstantsType::EndProcessing:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d %d 0 %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node,
        log->msg_len,
        log->idx1,
        log->idx2,
        log->idx3,
        log->idx4
      );
      break;
    case TraceConstantsType::BeginIdle:
      gzprintf(
        file,
        "%d %lld %d\n",
        type,
        converted_time,
        log->node
      );
      break;
    case TraceConstantsType::EndIdle:
      gzprintf(
        file,
        "%d %lld %d\n",
        type,
        converted_time,
        log->node
      );
      break;
    case TraceConstantsType::CreationBcast:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d %d %d %d\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node,
        log->msg_len,
        0,
        num_nodes
      );
      break;
    case TraceConstantsType::Creation:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d %d 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node,
        log->msg_len
      );
      break;
    case TraceConstantsType::UserEvent:
    case TraceConstantsType::UserEventPair:
    case TraceConstantsType::BeginUserEventPair:
    case TraceConstantsType::EndUserEventPair:
      gzprintf(
        file,
        "%d %lld %lld %d %d %d\n",
        type,
        log->user_event,
        converted_time,
        log->event,
        log->node,
        0
      );
      break;
    case TraceConstantsType::UserSupplied:
      gzprintf(
        file,
        "%d %d %lld\n",
        type,
        log->user_supplied_data,
        converted_time
      );
      break;
    case TraceConstantsType::UserSuppliedNote:
      gzprintf(
        file,
        "%d %lld %zu %s\n",
        type,
        converted_time,
        log->user_supplied_note.length(),
        log->user_supplied_note.c_str()
      );
      break;
    case TraceConstantsType::UserSuppliedBracketedNote: {
      auto const converted_end_time = timeToInt(log->end_time - start_time_);
      gzprintf(
        file,
        "%d %lld %lld %d %zu %s\n",
        type,
        converted_time,
        converted_end_time,
        log->event,
        log->user_supplied_note.length(),
        log->user_supplied_note.c_str()
      );
      break;
    }
    case TraceConstantsType::MessageRecv:
      vtAssert(false, "Message receive log type unimplemented");
      break;
    default:
      vtAssertInfo(false, "Unimplemented log type", converted_time, log->node);
    }

    // LogType is released - however, the collection itself is
    // not trimmed and will grow without bounds fsvo growth.
    traces[i].reset(nullptr);
  }

  cur_ = stop_point;
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
  NodeType const node, double const start, vt_gzFile *file
) {
  // Output header for projections file
  gzprintf(file->file_type, "PROJECTIONS-RECORD 0\n");
  // '6' means COMPUTATION_BEGIN to Projections: this starts a trace
  gzprintf(file->file_type, "6 0\n");
}

/*static*/ void Trace::outputFooter(
  NodeType const node, double const start, vt_gzFile *file
) {
  // Output footer for projections file, '7' means COMPUTATION_END to
  // Projections
  gzprintf(file->file_type, "7 %lld\n", timeToInt(getCurrentTime() - start));
}

}} //end namespace vt::trace
