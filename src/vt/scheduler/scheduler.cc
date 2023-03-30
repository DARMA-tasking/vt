/*
//@HEADER
// *****************************************************************************
//
//                                 scheduler.cc
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

#include "vt/config.h"
#include "vt/scheduler/scheduler.h"
#include "vt/messaging/active.h"
#include "vt/event/event.h"
#include "vt/termination/termination.h"
#include "vt/vrt/collection/manager.h"
#include "vt/objgroup/manager.fwd.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/utils/memory/memory_usage.h"
#include "vt/runtime/runtime.h"
#include "vt/runtime/mpi_access.h"
#include "vt/scheduler/thread_manager.h"

namespace vt { namespace sched {

/*static*/ void Scheduler::checkTermSingleNode() {
  auto const& num_nodes = theContext()->getNumNodes();
  if (num_nodes == 1) {
    theTerm()->maybePropagate();
  }
}

Scheduler::Scheduler()
#if vt_check_enabled(fcontext)
  : thread_manager_(std::make_unique<ThreadManager>())
#endif
{
  auto event_count = SchedulerEventType::LastSchedulerEvent + 1;
  event_triggers.resize(event_count);
  event_triggers_once.resize(event_count);

  progress_time_enabled_ = theConfig()->vt_sched_progress_sec != 0.0;

  // Number of times the progress function is called to poll components
  progressCount = registerCounter("num_progress", "progress function calls");

  // Number of work units enqueued
  workUnitCount = registerCounter("num_work_units", "work unit count");

  // Max/avg of work units enqueued
  queueSizeGauge = registerGauge("queue_size", "work queue size");

  // Time scheduler
  vtLiveTime = registerTimer("init_time", "duration VT was initialized");
  schedLoopTime = registerTimer("sched_loop", "inside scheduler loop");
  idleTime = registerTimer("idle_time", "idle time (inc. TD)");
  idleTimeMinusTerm = registerTimer("idle_time_term", "idle time (exc. TD)");

  // Explicitly define these out when diagnostics are disabled---they might be
  // expensive
# if vt_check_enabled(diagnostics)
  // Triggers to get the in-scheduler-loop time added to diagnostics
  registerTrigger(BeginSchedulerLoop, [this]{ schedLoopTime.start(); });
  registerTrigger(EndSchedulerLoop, [this]{ schedLoopTime.stop(); });

  // Triggers to get true idle time (including TD messages) in diagnostics
  registerTrigger(BeginIdle, [this]{ idleTime.start(); });
  registerTrigger(EndIdle, [this]{ idleTime.stop(); });

  // Triggers to get non-term idle time (excluding TD messages) in diagnostics
  registerTrigger(BeginIdleMinusTerm, [this]{ idleTimeMinusTerm.start(); });
  registerTrigger(EndIdleMinusTerm, [this]{ idleTimeMinusTerm.stop(); });

  vtLiveTime.start();
# endif
}

void Scheduler::startup() {
  special_progress_ =
    theConfig()->vt_sched_progress_han != 0 or progress_time_enabled_;
}

void Scheduler::preDiagnostic() {
  vtLiveTime.stop();
}

void Scheduler::runWorkUnit(UnitType& work) {
  bool const is_term = work.isTerm();

#if vt_check_enabled(mpi_access_guards)
  vtAssert(
    not vt::runtime::ScopedMPIAccess::isExplicitlyGranted(),
    "Explicit MPI access should not be active when executing work unit."
  );
  if (action_depth_ == 0) { // 0 -> 1 transition before work unit executed
    vt::runtime::ScopedMPIAccess::prohibitByDefault(true);
  }
#endif

  workUnitCount.increment(1);

#if vt_check_enabled(mpi_access_guards)
  ++action_depth_;
#endif

  work();

#if vt_check_enabled(mpi_access_guards)
  --action_depth_;
#endif

#if vt_check_enabled(mpi_access_guards)
  if (action_depth_ == 0) {
    vt::runtime::ScopedMPIAccess::prohibitByDefault(false);
  }
#endif

  if (is_term) {
    --num_term_msgs_;
  }
}

/*private*/
bool Scheduler::progressImpl(TimeType current_time) {
  int const total = curRT->progress(current_time);

  checkTermSingleNode();

  return total != 0;
}

/*private*/
bool Scheduler::progressMsgOnlyImpl(TimeType current_time) {
  return theMsg()->progress(current_time) or theEvent()->progress(current_time);
}

bool Scheduler::shouldCallProgress(
  int32_t processed_since_last_progress, TimeType time_since_last_progress
) const {

  // By default, `vt_sched_progress_han` is 0 and will happen every time we go
  // through the scheduler
  bool k_handler_enabled = theConfig()->vt_sched_progress_han != 0;
  bool k_handlers_executed =
    k_handler_enabled and
    processed_since_last_progress >= theConfig()->vt_sched_progress_han;
  bool enough_time_passed =
    progress_time_enabled_ and
    time_since_last_progress > theConfig()->vt_sched_progress_sec;

  return
    (not progress_time_enabled_ and not k_handler_enabled) or
    enough_time_passed or k_handlers_executed;
}

void Scheduler::printMemoryUsage() {
  if (
    last_memory_usage_poll_ >=
    static_cast<std::size_t>(theConfig()->vt_print_memory_sched_poll)
  ) {
    auto usage = theMemUsage();

    if (usage != nullptr) {
      if (threshold_memory_usage_ == 0) {
        threshold_memory_usage_ = usage->convertBytesFromString(
          theConfig()->vt_print_memory_threshold
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

void Scheduler::runProgress(bool msg_only, TimeType current_time) {
  /*
   * Run through the progress functions `num_iter` times, making forward
   * progress on MPI
   */
  auto const num_iter = std::max(1, theConfig()->vt_sched_num_progress);
  for (int i = 0; i < num_iter; i++) {
    if (msg_only) {
      // This is a special case used only during startup when other components
      // are not ready and progress should not be called on them.
      progressMsgOnlyImpl(current_time);
    } else {
      progressImpl(current_time);
    }
    progressCount.increment(1);
  }

  if (theConfig()->vt_print_memory_at_threshold) {
    printMemoryUsage();
  }
  is_recent_time_stale_ = true;
  if (special_progress_) {
    // Reset count of processed handlers since the last time progress was invoked
    processed_after_last_progress_ = 0;
    last_progress_time_ = getRecentTime();
  }
}

void Scheduler::runSchedulerOnceImpl(bool msg_only) {
  if (special_progress_) {
    auto current_time = getRecentTime();
    auto time_since_last_progress = current_time - last_progress_time_;
    if (shouldCallProgress(processed_after_last_progress_, time_since_last_progress)) {
      runProgress(msg_only, current_time);
    }
  } else if (work_queue_.empty()) {
    if (curRT->needsCurrentTime()) {
      runProgress(msg_only, getRecentTime());
    } else {
      runProgress(msg_only, TimeType{0});
    }
  }

  if (not work_queue_.empty()) {
    queueSizeGauge.update(work_queue_.size());

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
  } else {
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

Scheduler::SchedulerLoopGuard::SchedulerLoopGuard(Scheduler* scheduler)
  : scheduler_{scheduler} {
  scheduler_->triggerEvent(SchedulerEventType::BeginSchedulerLoop);
}

Scheduler::SchedulerLoopGuard::~SchedulerLoopGuard() {
  scheduler_->triggerEvent(SchedulerEventType::EndSchedulerLoop);
}

void Scheduler::runSchedulerWhile(std::function<bool()> cond) {
  // This loop construct can run either in a top-level or nested context.
  // 1. In a top-level context the idle time will encompass the time not
  //    processing any actions when the work queue is empty. Leaving starts
  //    a 'between scheduler' event which will only complete on the next call.
  // 2. In a nested context, idle is always exited at the end
  //    as the parent context is "not idle". Likewise, no 'between scheduler'
  //    event is started.

  SchedulerLoopGuard loopGuard{this};

  // Ensure to immediately enter an idle state if such applies.
  // The scheduler call ends idle as picking up work.
  if (not is_idle and work_queue_.empty()) {
    is_idle = true;
    triggerEvent(SchedulerEventType::BeginIdle);
  }

  while (cond()) {
    runSchedulerOnceImpl();
  }

  // After running the scheduler ensure to exit idle state.
  // For nested schedulers the outside scheduler has work to do.
  // Between top-level schedulers, non-idle indicates lack of scheduling.
  if (is_idle) {
    is_idle = false;
    triggerEvent(SchedulerEventType::EndIdle);
  }
}

void Scheduler::triggerEvent(SchedulerEventType const& event) {
  vtAssert(
    event_triggers.size() >= static_cast<size_t>(event), "Must be large enough to hold this event"
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
    event_triggers.size() >= static_cast<size_t>(event), "Must be large enough to hold this event"
  );
  event_triggers[event].push_back(trigger);
}

void Scheduler::registerTriggerOnce(
  SchedulerEventType const& event, TriggerType trigger
) {
  vtAssert(
    event_triggers.size() >= static_cast<size_t>(event), "Must be large enough to hold this event"
  );
  event_triggers_once[event].push_back(trigger);
}

void Scheduler::suspend(
  ThreadIDType tid, RunnablePtrType runnable, PriorityType p
) {
  suspended_.addSuspended(tid, std::move(runnable), p);
}

void Scheduler::resume(ThreadIDType tid) {
  suspended_.resumeRunnable(tid);
}

#if vt_check_enabled(fcontext)
ThreadManager* Scheduler::getThreadManager() {
  return thread_manager_.get();
}
#endif

}} //end namespace vt::sched

namespace vt {

void runSchedulerThrough(EpochType epoch) {
  // WARNING: This is to prevent global termination from spuriously
  // thinking that the work done in this loop over the scheduler
  // represents the entire work of the program, and thus leading to
  // stuff being torn down
  theTerm()->produce();
  theSched()->runSchedulerWhile([=]{ return !theTerm()->isEpochTerminated(epoch); });
  theTerm()->consume();
}

} //end namespace vt
