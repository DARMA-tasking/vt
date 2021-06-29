/*
//@HEADER
// *****************************************************************************
//
//                                   trace.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/trace/file_spec/spec.h"
#include "vt/objgroup/headers.h"
#include "vt/utils/memory/memory_usage.h"
#include "vt/phase/phase_manager.h"
#include "vt/runtime/runtime.h"

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

using LogType = Trace::LogType;

Trace::Trace(std::string const& in_prog_name) : TraceLite(in_prog_name)
{}

void Trace::initialize() /*override*/ {
#if vt_check_enabled(trace_enabled)
  setupNames(prog_name_);

  between_sched_event_type_ = trace::TraceRegistry::registerEventHashed(
    "Scheduler", "Between_Schedulers"
  );

  // Register a trace user event to demarcate flushes that occur
  flush_event_ = trace::registerEventCollective("trace_flush");
#endif
}

void Trace::startup() /*override*/ {
#if vt_check_enabled(trace_enabled)
  theSched()->registerTrigger(
    sched::SchedulerEvent::PendingSchedulerLoop, [this]{ pendingSchedulerLoop(); }
  );
  theSched()->registerTrigger(
    sched::SchedulerEvent::BeginSchedulerLoop, [this]{ beginSchedulerLoop(); }
  );
  theSched()->registerTrigger(
    sched::SchedulerEvent::EndSchedulerLoop, [this]{ endSchedulerLoop(); }
  );
  theSched()->registerTrigger(
    sched::SchedulerEvent::BeginIdle, [this]{ beginIdle(); }
  );
  theSched()->registerTrigger(
    sched::SchedulerEvent::EndIdle, [this]{ endIdle(); }
  );

  thePhase()->registerHookRooted(phase::PhaseHook::End, [] {
    auto const phase = thePhase()->getCurrentPhase();
    theTrace()->setTraceEnabledCurrentPhase(phase + 1);
  });
  thePhase()->registerHookCollective(phase::PhaseHook::EndPostMigration, [] {
    theTrace()->flushTracesFile(false);
  });
#endif
}

void Trace::finalize() /*override*/ {
  // Always end any between-loop event left open.
  endProcessing(between_sched_event_);
  between_sched_event_ = TraceProcessingTag{};
}

void Trace::loadAndBroadcastSpec() {
  if (theConfig()->vt_trace_spec) {
    auto spec_proxy = file_spec::TraceSpec::construct();

    theTerm()->produce();
    if (theContext()->getNode() == 0) {
      auto spec_ptr = spec_proxy.get();
      spec_ptr->parse();
      spec_ptr->broadcastSpec();
    }
    theSched()->runSchedulerWhile([&spec_proxy]{
      return not spec_proxy.get()->specReceived();
    });
    theTerm()->consume();

    spec_proxy_ = spec_proxy.getProxy();

    // Set enabled for the initial phase
    setTraceEnabledCurrentPhase(0);
  }
}

bool Trace::inIdleEvent() const {
  return idle_begun_;
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

  vt_debug_print(
    normal, trace,
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

  vt_debug_print(
    normal, trace,
    "Trace::addUserData: data={}\n",
    data
  );

  auto const type = TraceConstantsType::UserSupplied;
  auto const time = getCurrentTime();

  logEvent(
    LogType{time, type, std::string{}, data}
  );
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
  #if vt_check_enabled(trace_enabled)
    theTrace()->user_event_.insertEvent(event, name);
  #endif
}

void Trace::addUserEvent(UserEventIDType event) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  vt_debug_print(
    normal, trace,
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

  vt_debug_print(
    normal, trace,
    "Trace::addUserEventManual: event={:x}\n",
    event
  );

  auto id = user_event_.createEvent(true, false, 0, event);
  addUserEvent(id);
}

void Trace::addUserEventBracketedBegin(UserEventIDType event) {
  if (not checkDynamicRuntimeEnabled()) {
    return;
  }

  vt_debug_print(
    normal, trace,
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

  vt_debug_print(
    normal, trace,
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

  vt_debug_print(
    normal, trace,
    "Trace::addUserEventBracketedManual: event={:x}, begin={}, end={}\n",
    event, begin, end
  );

  auto id = user_event_.createEvent(true, false, 0, event);
  addUserEventBracketed(id, begin, end);
}

void Trace::addMemoryEvent(std::size_t memory, double time) {
  auto const type = TraceConstantsType::MemoryUsageCurrent;
  logEvent(LogType{time, type, memory});
}

TraceProcessingTag Trace::beginProcessing(
  TraceEntryIDType const ep, TraceMsgLenType const len,
  TraceEventIDType const event, NodeType const from_node,
  uint64_t const idx1, uint64_t const idx2,
  uint64_t const idx3, uint64_t const idx4,
  double const time
) {
  if (not checkDynamicRuntimeEnabled()) {
    return TraceProcessingTag{};
  }

  vt_debug_print(
    normal, trace,
    "event_start: ep={}, event={}, time={}, from={}, entry chare={}\n",
    ep, event, time, from_node, TraceRegistry::getEvent(ep).theEventSeq()
  );

  auto const type = TraceConstantsType::BeginProcessing;

  emitTraceForTopProcessingEvent(time, TraceConstantsType::EndProcessing);
  TraceEventIDType loggedEvent = logEvent(
    LogType{
      time, ep, type, event, len, from_node, idx1, idx2, idx3, idx4
    }
  );

  if (theConfig()->vt_trace_memory_usage) {
    addMemoryEvent(theMemUsage()->getFirstUsage());
  }

  return TraceProcessingTag{ep, loggedEvent};
}

void Trace::endProcessing(
  TraceProcessingTag const& processing_tag,
  double const time
) {
  // End event honored even if tracing is disabled in this phase.
  // This ensures proper stack unwinding in all contexts.
  if (not checkDynamicRuntimeEnabled(true)) {
    return;
  }

  TraceEntryIDType ep = processing_tag.ep_;
  TraceEventIDType event = processing_tag.event_;

  // Allow no-op cases (ie. beginProcessing disabled)
  if (ep == trace::no_trace_entry_id) {
    return;
  }

  if (idle_begun_) {
    // TODO: This should be a prohibited case - vt 1.1?
    endIdle(time);
  }

  vt_debug_print(
    normal, trace,
    "event_stop: ep={}, event={}, time={}, from_node={}, entry chare={}\n",
    ep, event, time, open_events_.back().node,
    TraceRegistry::getEvent(ep).theEventSeq()
  );

  vtAssert(
    not open_events_.empty()
    // This is current contract expectations; however it precludes async closing.
    and open_events_.back().ep == ep
    and open_events_.back().event == event,
    "Event being closed must be on the top of the open event stack."
  );

  if (theConfig()->vt_trace_memory_usage) {
    addMemoryEvent(theMemUsage()->getFirstUsage());
  }

  // Final event is same as original with a few .. tweaks.
  // Always done PRIOR TO restarts.
  traces_.push(
    LogType{open_events_.back(), time, TraceConstantsType::EndProcessing}
  );
  open_events_.pop_back();
  emitTraceForTopProcessingEvent(time, TraceConstantsType::BeginProcessing);

  // Unlike logEvent there is currently no flush here.
}

void Trace::pendingSchedulerLoop() {
  // Always end between-loop event.
  endProcessing(between_sched_event_);
  between_sched_event_ = TraceProcessingTag{};
}

void Trace::beginSchedulerLoop() {
  // Always end between-loop event. The pending case is not always triggered.
  endProcessing(between_sched_event_);
  between_sched_event_ = TraceProcessingTag{};

  // Capture the current open event depth.
  event_holds_.push_back(open_events_.size());
}

void Trace::endSchedulerLoop() {
  vtAssert(
    event_holds_.size() >= 1,
    "Too many endSchedulerLoop calls."
  );

  vtAssert(
    event_holds_.back() == open_events_.size(),
    "Processing events opened in a scheduler loop must be closed by loop end."
  );

  event_holds_.pop_back();

  // Start an event representing time outside of top-level scheduler.
  if (event_holds_.size() == 1) {
    between_sched_event_ = beginProcessing(
      between_sched_event_type_, 0, trace::no_trace_event, 0
    );
  }
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

void Trace::setTraceEnabledCurrentPhase(PhaseType cur_phase) {
  if (spec_proxy_ != vt::no_obj_group) {
    // SpecIndex is signed due to negative/positive, phase is not signed
    auto spec_index = static_cast<file_spec::TraceSpec::SpecIndex>(cur_phase);
    vt::objgroup::proxy::Proxy<file_spec::TraceSpec> proxy(spec_proxy_);
    bool ret = proxy.get()->checkTraceEnabled(spec_index);

    if (trace_enabled_cur_phase_ != ret) {
      // N.B. Future endProcessing calls are required to close the current
      // open event stack, even after the tracing is disabled for the phase.
      // This ensures valid stack unwinding in all cases.

      // Go ahead and perform a trace flush when tracing is disabled (and was
      // previously enabled) to reduce memory footprint.
      if (not ret and theConfig()->vt_trace_flush_size != 0) {
        writeTracesFile(incremental_flush_mode, true);
      }
    }

    trace_enabled_cur_phase_ = ret;

    vt_debug_print(
      terse, gen,
      "setTraceEnabledCurrentPhase: phase={}, enabled={}\n",
      cur_phase,
      trace_enabled_cur_phase_
    );
  }
}

}} //end namespace vt::trace
