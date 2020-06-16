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
#include "vt/utils/memory/memory_usage.h"
#include "vt/runtime/runtime.h"
#include "vt/runtime/mpi_access.h"

namespace vt { namespace sched {

/*static*/ void Scheduler::checkTermSingleNode() {
  auto const& num_nodes = theContext()->getNumNodes();
  if (num_nodes == 1) {
    theTerm()->maybePropagate();
  }
}

Scheduler::Scheduler() {
  auto event_count = SchedulerEventType::LastSchedulerEvent + 1;
  event_triggers.resize(event_count);
  event_triggers_once.resize(event_count);

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

void Scheduler::runWorkUnit(UnitType& work) {
  bool const is_term = work.isTerm();

#if backend_check_enabled(mpi_access_guards)
  vtAssert(
    not vt::runtime::ScopedMPIAccess::isExplicitlyGranted(),
    "Explicit MPI access should not be active when executing work unit."
  );
  if (action_depth_ == 0) { // 0 -> 1 transition before work unit executed
    vt::runtime::ScopedMPIAccess::prohibitByDefault(true);
  }
#endif

  ++action_depth_;
  work();
  --action_depth_;

#if backend_check_enabled(mpi_access_guards)
  if (action_depth_ == 0) {
    vt::runtime::ScopedMPIAccess::prohibitByDefault(false);
  }
#endif

  if (is_term) {
    --num_term_msgs_;
  }
}

/*private*/
bool Scheduler::progressImpl() {
  int const total = curRT->progress();

  checkTermSingleNode();

  return total != 0;
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

void Scheduler::printMemoryUsage() {
  if (
    last_memory_usage_poll_ >=
    static_cast<std::size_t>(arguments::ArgConfig::vt_print_memory_sched_poll)
  ) {
    auto usage = theMemUsage();

    if (usage != nullptr) {
      if (threshold_memory_usage_ == 0) {
        threshold_memory_usage_ = usage->convertBytesFromString(
          arguments::ArgConfig::vt_print_memory_threshold
        );
      }

      auto cur_bytes = usage->getFirstUsage();
      if (cur_bytes > last_threshold_memory_usage_ + threshold_memory_usage_) {
        vt_print(gen, "Memory usage (+) {}\n", usage->getUsageAll());
        last_threshold_memory_usage_ = cur_bytes;
      } else if (
        last_threshold_memory_usage_ > threshold_memory_usage_ and
        cur_bytes < last_threshold_memory_usage_ - threshold_memory_usage_
      ) {
        vt_print(gen, "Memory usage (-) {}\n", usage->getUsageAll());
        last_threshold_memory_usage_ = cur_bytes;
      }
    }

    last_memory_usage_poll_ = 0;
  } else {
    last_memory_usage_poll_++;
  }
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

  if (arguments::ArgConfig::vt_print_memory_at_threshold) {
    printMemoryUsage();
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

  if (not work_queue_.empty()) {
    processed_after_last_progress_++;

    // Leave idle states are before any potential processing.
    // True-idle must be the outer state to enter/leave to collect more
    // accurate time and ensure that events are not emited while in idle.
    if (is_idle) {
      is_idle = false;
      triggerEvent(SchedulerEventType::EndIdle);
    }
    if (is_idle_minus_term and num_term_msgs_ not_eq work_queue_.size()) {
      is_idle_minus_term = false;
      triggerEvent(SchedulerEventType::EndIdleMinusTerm);
    }

    /*
     * Run a work unit!
     */
    UnitType work = work_queue_.pop();
    runWorkUnit(work);

    // Enter idle state immediately after processing if relevant.
    if (not is_idle_minus_term and num_term_msgs_ == work_queue_.size()) {
      is_idle_minus_term = true;
      triggerEvent(SchedulerEventType::BeginIdleMinusTerm);
    }
    if (not is_idle and work_queue_.empty()) {
      is_idle = true;
      triggerEvent(SchedulerEventType::BeginIdle);
    }
  }

  if (not msg_only) {
    has_executed_ = true;
  }
}

void Scheduler::runSchedulerWhile(std::function<bool()> cond) {
  // This loop construct can run either in a top-level or nested context.
  // 1. In a top-level context the idle time will encompass the time not
  //    processing any actions when the work queue is empty. Leaving starts
  //    a 'between scheduler' event which will only complete on the next call.
  // 2. In a nested context, idle is always exited at the end
  //    as the parent context is "not idle". Likewise, no 'between scheduler'
  //    event is started.

  vtAssert(
    action_depth_ == 0 or not is_idle,
    "Nested schedulers never expected from idle context"
  );

  triggerEvent(SchedulerEventType::BeginSchedulerLoop);

  // When resuming a top-level scheduler, ensure to immediately enter
  // an idle state if such applies.
  if (not is_idle and work_queue_.empty()) {
    is_idle = true;
    triggerEvent(SchedulerEventType::BeginIdle);
  }

  while (cond()) {
    scheduler();
  }

  // After running the scheduler ensure to exit idle state.
  // For nested schedulers the outside scheduler has work to do.
  // Between top-level schedulers, non-idle indicates lack of scheduling.
  if (is_idle) {
    is_idle = false;
    triggerEvent(SchedulerEventType::EndIdle);
  }

  triggerEvent(SchedulerEventType::EndSchedulerLoop);
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

}} //end namespace vt::sched

namespace vt {

void runScheduler() {
  theSched()->scheduler();
}

void runSchedulerThrough(EpochType epoch) {
  // WARNING: This is to prevent global termination from spuriously
  // thinking that the work done in this loop over the scheduler
  // represents the entire work of the program, and thus leading to
  // stuff being torn down
  theTerm()->produce();
  theSched()->runSchedulerWhile([=]{ return !theTerm()->isEpochTerminated(epoch); });
  theTerm()->consume();
}

void runInEpochRooted(ActionType&& fn) {
  theSched()->triggerEvent(sched::SchedulerEvent::PendingSchedulerLoop);

  auto ep = theTerm()->makeEpochRooted();
  theMsg()->pushEpoch(ep);
  fn();
  theMsg()->popEpoch(ep);
  theTerm()->finishedEpoch(ep);
  runSchedulerThrough(ep);
}

void runInEpochCollective(ActionType&& fn) {
  theSched()->triggerEvent(sched::SchedulerEvent::PendingSchedulerLoop);

  auto ep = theTerm()->makeEpochCollective();
  theMsg()->pushEpoch(ep);
  fn();
  theMsg()->popEpoch(ep);
  theTerm()->finishedEpoch(ep);
  runSchedulerThrough(ep);
}

} //end namespace vt
