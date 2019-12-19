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

bool Scheduler::runNextUnit() {
  if (not work_queue_.empty()) {
    auto elm = work_queue_.pop();
    bool const is_term = elm.isTerm();
    elm();
    if (is_term) {
      num_term_msgs_--;
    }
    return true;
  } else {
    return false;
  }
}

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

bool Scheduler::progressMsgOnlyImpl() {
  return theMsg()->progress() or theEvent()->progress();
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
  using ArgType  = arguments::ArgConfig;
  using TimeType = timing::Timing;

  // By default, `vt_sched_progress_han` is 0 and will happen every time we go
  // through the scheduler
  bool k_handler_enabled = ArgType::vt_sched_progress_han != 0;
  bool k_handlers_executed =
    k_handler_enabled and
    processed_after_last_progress_ >= ArgType::vt_sched_progress_han;
  bool enough_time_passed =
    progress_time_enabled_ and
    TimeType::getCurrentTime() - last_progress_time_ > ArgType::vt_sched_progress_sec;

  if ((not progress_time_enabled_ and not k_handler_enabled) or
      enough_time_passed or k_handlers_executed) {
    runProgress(msg_only);
  }

  /*
   * Handle cases when the system goes from term-idle to non-term-idle; trigger
   * user registered listeners
   */
  if (num_term_msgs_ == work_queue_.size()) {
    if (not is_idle_minus_term) {
      is_idle_minus_term = true;
      triggerEvent(SchedulerEventType::BeginIdleMinusTerm);
    }
  } else {
    if (is_idle_minus_term) {
      is_idle_minus_term = false;
      triggerEvent(SchedulerEventType::EndIdleMinusTerm);
    }
  }

  /*
   * Handle cases when the system goes from completely idle or not; trigger user
   * registered listeners
   */
  if (work_queue_.empty()) {
    if (not is_idle) {
      is_idle = true;
      // Trigger any registered listeners on idle
      triggerEvent(SchedulerEventType::BeginIdle);
    }
  } else {
    if (is_idle) {
      is_idle = false;
      triggerEvent(SchedulerEventType::EndIdle);
    }
  }

  /*
   * Run a work unit!
   */
  if (not work_queue_.empty()) {
    processed_after_last_progress_++;
    runNextUnit();
  }

  if (not msg_only) {
    has_executed_ = true;
  }
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

void Scheduler::schedulerForever() {
  while (true) {
    scheduler();
  }
}

}} //end namespace vt::scheduler

namespace vt {

void runScheduler() {
  theSched()->scheduler();
}

} //end namespace vt
