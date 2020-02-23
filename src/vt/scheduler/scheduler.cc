/*
//@HEADER
// *****************************************************************************
//
//                                 scheduler.cc
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

#include "vt/config.h"
#include "vt/scheduler/scheduler.h"
#include "vt/messaging/active.h"
#include "vt/event/event.h"
#include "vt/termination/termination.h"
#include "vt/sequence/sequencer.h"
#include "vt/sequence/sequencer_virtual.h"
#include "vt/worker/worker_headers.h"
#include "vt/vrt/collection/manager.h"
#include "vt/objgroup/manager.fwd.h"
#include "vt/configs/arguments/args.h"

namespace vt { namespace sched {

/*static*/ void Scheduler::checkTermSingleNode() {
  auto const& num_nodes = theContext()->getNumNodes();
  if (num_nodes == 1) {
    theTerm()->maybePropagate();
  }
}

Scheduler::Scheduler() {
  event_triggers.resize(SchedulerEventType::SchedulerEventSize + 1);
  event_triggers_once.resize(SchedulerEventType::SchedulerEventSize + 1);
  progress_time_enabled_ = arguments::ArgConfig::vt_sched_progress_sec != 0.0;
}

void Scheduler::enqueue(ActionType action) {
  bool const is_term = false;
# if backend_check_enabled(priorities)
  work_queue_.emplace(UnitType(is_term, default_priority, action));
# else
  work_queue_.emplace(UnitType(is_term, action));
# endif
}

void Scheduler::enqueue(PriorityType priority, ActionType action) {
  bool const is_term = false;
# if backend_check_enabled(priorities)
  work_queue_.emplace(UnitType(is_term, priority, action));
# else
  work_queue_.emplace(UnitType(is_term, action));
# endif
}

/*private*/
bool Scheduler::runWorkUnit(UnitType& work) {
  bool const is_term = work.isTerm();

  ++unit_run_depth_;
  work();
  --unit_run_depth_;

  if (is_term) {
    --num_term_msgs_;
  }

  return unit_run_depth_ == 0;
}

/*private*/
bool Scheduler::progressImpl() {
  bool const msg_sch = theMsg()->progress();
  bool const evt_sch = theEvent()->progress();
  bool const seq_sch = theSeq()->progress();
  bool const vrt_sch = theVirtualSeq()->progress();
  bool const col_sch = theCollection()->progress();
  bool const obj_sch = theObjGroup()->progress();

  bool const worker_sch =
    theContext()->hasWorkers() ? theWorkerGrp()->progress(),false : false;
  bool const worker_comm_sch =
    theContext()->hasWorkers() ? theWorkerGrp()->commScheduler() : false;

  checkTermSingleNode();

  bool scheduled_work =
    msg_sch or evt_sch or seq_sch or vrt_sch or
    col_sch or obj_sch or worker_sch or worker_comm_sch;

  return scheduled_work;
}

/*private*/
bool Scheduler::progressMsgOnlyImpl() {
  return theMsg()->progress() or theEvent()->progress();
}

bool Scheduler::shouldCallProgress(
  int32_t processed_since_last_progress, TimeType time_since_last_progress
) const {
  using ArgType   = arguments::ArgConfig;

  // By default, `vt_sched_progress_han` is 0 and will happen every time we go
  // through the scheduler
  bool k_handler_enabled = ArgType::vt_sched_progress_han != 0;
  bool k_handlers_executed =
    k_handler_enabled and
    processed_since_last_progress >= ArgType::vt_sched_progress_han;
  bool enough_time_passed =
    progress_time_enabled_ and
    time_since_last_progress > ArgType::vt_sched_progress_sec;

  return
    (not progress_time_enabled_ and not k_handler_enabled) or
    enough_time_passed or k_handlers_executed;
}

void Scheduler::runProgress(bool msg_only) {
  /*
   * Run through the progress functions `num_iter` times, making forward
   * progress on MPI
   */
  auto const num_iter = std::max(1, arguments::ArgConfig::vt_sched_num_progress);
  for (int i = 0; i < num_iter; i++) {
    if (msg_only) {
      // This is a special case used only during startup when other components
      // are not ready and progress should not be called on them.
      progressMsgOnlyImpl();
    } else {
      progressImpl();
    }
  }

  // Reset count of processed handlers since the last time progress was invoked
  processed_after_last_progress_ = 0;
  last_progress_time_ = timing::Timing::getCurrentTime();
}

void Scheduler::scheduler(bool msg_only) {
  using TimerType = timing::Timing;

  auto time_since_last_progress = TimerType::getCurrentTime() - last_progress_time_;
  if (
    work_queue_.empty() or
    shouldCallProgress(processed_after_last_progress_, time_since_last_progress)
  ) {
    runProgress(msg_only);
  }

  /*
   * Run a work unit!
   */
  if (not work_queue_.empty()) {
    // Leave idle states are before any potential processing.
    if (is_idle_minus_term and num_term_msgs_ not_eq work_queue_.size()) {
      is_idle_minus_term = false;
      triggerEvent(SchedulerEventType::EndIdleMinusTerm);
    }
    if (is_idle) {
      is_idle = false;
      triggerEvent(SchedulerEventType::EndIdle);
    }

    processed_after_last_progress_++;

    UnitType work = work_queue_.pop();
    bool top_scheduler = runWorkUnit(work);

    // Enter idle state immediately after processing if relevant.
    if (top_scheduler and not is_idle and work_queue_.empty()) {
      is_idle = true;
      triggerEvent(SchedulerEventType::BeginIdle);
    }
    if (not is_idle_minus_term and num_term_msgs_ == work_queue_.size()) {
      is_idle_minus_term = true;
      triggerEvent(SchedulerEventType::BeginIdleMinusTerm);
    }
  }

  if (not msg_only) {
    has_executed_ = true;
  }
}

SchedulerRunner Scheduler::beginScheduling(
  SchedulerTypeID scheduler_tag
) {
  return SchedulerRunner{unit_run_depth_, scheduler_tag};
}

void Scheduler::triggerEvent(SchedulerEventType const& event) {
  vtAssert(
    event_triggers.size() >= event, "Must be large enough to hold this event"
  );

  for (auto& t : event_triggers[event]) {
    t();
  }

  for (auto& t : event_triggers_once[event]) {
    t();
  }
  event_triggers_once[event].clear();
}

void Scheduler::registerTrigger(
  SchedulerEventType const& event, TriggerType trigger
) {
  vtAssert(
    event_triggers.size() >= event, "Must be large enough to hold this event"
  );
  event_triggers[event].push_back(trigger);
}

void Scheduler::registerTriggerOnce(
  SchedulerEventType const& event, TriggerType trigger
) {
  vtAssert(
    event_triggers.size() >= event, "Must be large enough to hold this event"
  );
  event_triggers_once[event].push_back(trigger);
}

SchedulerRunner::SchedulerRunner(
  unsigned int in_sched_depth, SchedulerTypeID in_sched_tag
) : sched_depth_(in_sched_depth), sched_tag_(in_sched_tag) {
}

SchedulerRunner::~SchedulerRunner() {
  theTrace()->endProcessing(processing_event_);
}

bool SchedulerRunner::runScheduler() {

#if backend_check_enabled(trace_enabled)
  // It might be more ideal to have a dedicated pairing event.
  // n.b. static here.
  static trace::TraceEntryIDType ep
    = trace::TraceRegistry::registerEventHashed("Scheduler", "RunScheduler");

  // Even though such "should" only be called from a nested context,
  // it is possible (such as a barrier on startup) that this is invoked
  // outside of any scheduler execution stack.
  if (sched_depth_ > 0 and not processing_event_.hasEvent()) {
    auto node = theContext()->getNode();
    uint64_t idx1 = static_cast<uint64_t>(sched_tag_);
    uint64_t idx2 = sched_depth_;
    processing_event_ = theTrace()->beginProcessing(
      ep, 0, 0, node, idx1, idx2, 0, 0
    );
  }
#endif

  theSched()->scheduler();
  return true;
}

}} //end namespace vt::sched

namespace vt {

void runScheduler() {
  theSched()->scheduler();
}

} //end namespace vt
