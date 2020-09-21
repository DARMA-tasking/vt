/*
//@HEADER
// *****************************************************************************
//
//                                 scheduler.h
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

#if !defined INCLUDED_SCHEDULER_SCHEDULER_H
#define INCLUDED_SCHEDULER_SCHEDULER_H

#include "vt/config.h"
#include "vt/scheduler/queue.h"
#include "vt/scheduler/priority_queue.h"
#include "vt/scheduler/prioritized_work_unit.h"
#include "vt/scheduler/work_unit.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/timing/timing.h"
#include "vt/runtime/component/component_pack.h"

#include <cassert>
#include <vector>
#include <list>
#include <functional>
#include <memory>

namespace vt {
  void runScheduler();
  void runSchedulerThrough(EpochType epoch);

  template <typename Callable>
  void runInEpochRooted(Callable&& fn);

  template <typename Callable>
  void runInEpochCollective(Callable&& fn);
}

namespace vt { namespace sched {

enum SchedulerEvent {
  BeginIdle            = 0,
  EndIdle              = 1,
  BeginIdleMinusTerm   = 2,
  EndIdleMinusTerm     = 3,
  BeginSchedulerLoop   = 4,
  EndSchedulerLoop     = 5,
  PendingSchedulerLoop = 6,

  LastSchedulerEvent   = 6,
};

/**
 * \struct Scheduler
 *
 * \brief A core VT component that schedules work generated from other
 * components. The pieces of work waiting to execute may be prioritized, see
 * src/vt/scheduler/priority.h
 *
 * Tracks work to be completed, orders it by priority, and executes it. Polls
 * components for incoming work.
 */
struct Scheduler : runtime::component::Component<Scheduler> {
  using SchedulerEventType   = SchedulerEvent;
  using TriggerType          = std::function<void()>;
  using TriggerContainerType = std::list<TriggerType>;
  using EventTriggerContType = std::vector<TriggerContainerType>;

# if vt_check_enabled(priorities)
  using UnitType             = PriorityUnit;
# else
  using UnitType             = Unit;
# endif

  Scheduler();

  std::string name() override { return "Scheduler"; }

  void preDiagnostic() override;

  /**
   * \internal \brief Check for termination when running on a since node
   */
  static void checkTermSingleNode();

  /**
   * \internal \brief Check if progress function needs to be called
   *
   * \param[in] processed_since_last_progress number of units processed since
   * last progress
   * \param[in] time_since_last_progress time since last progress
   *
   * \return whether progress needs to be called
   */
  bool shouldCallProgress(
    int32_t processed_since_last_progress, TimeType time_since_last_progress
  ) const;

  /**
   * \brief Turn the scheduler
   *
   * Polls every component that might generate or complete work, and
   * potentially runs one piece of available work.
   *
   * \param[in] msg_only whether to only make progress on the core active
   * messenger
   */
  void scheduler(bool msg_only = false);

  /**
   * \brief Run the progress function
   *
   * \param[in] msg_only whether to only make progress on the core active
   * messenger
   */
  void runProgress(bool msg_only = false);

  /**
   * \brief Runs the scheduler until a condition is met.
   *
   * Runs the scheduler until a condition is met.
   * This form SHOULD be used instead of "while (..) { runScheduler(..) }"
   * in all cases of nested scheduler loops, such as during a barrier,
   * in order to ensure proper event unwinding and idle time tracking.
   *
   * \param[in] cond condition to turn scheduler until met
   */
  void runSchedulerWhile(std::function<bool()> cond);

  /**
   * \brief Register a trigger with the scheduler
   *
   * \param[in] event event to trigger on
   * \param[in] trigger function to trigger
   */
  void registerTrigger(SchedulerEventType const& event, TriggerType trigger);

  /**
   * \brief Register a trigger once with the scheduler
   *
   * \param[in] event event to trigger on
   * \param[in] trigger function to trigger
   */
  void registerTriggerOnce(
    SchedulerEventType const& event, TriggerType trigger
  );

  /**
   * \internal \brief Trigger an event
   *
   * \param[in] event the event to trigger
   */
  void triggerEvent(SchedulerEventType const& event);

  /**
   * \internal \brief Check if the scheduler has run, polling all components at
   * least once
   *
   * \return whether the scheduler has advanced
   */
  bool hasSchedRun() const { return has_executed_; }

  /**
   * \brief Enqueue an action to execute later with the default priority
   * \c default_priority
   *
   * \param[in] action action to execute
   */
  void enqueue(ActionType action);

  /**
   * \brief Enqueue an action with a priority to execute later
   *
   * \param[in] priority the priority of the action
   * \param[in] action the action to execute later
   */
  void enqueue(PriorityType priority, ActionType action);

  /**
   * \brief Print current memory usage
   */
  void printMemoryUsage();

  /**
   * \brief Enqueue an action associated with a prioritized message. The action
   * will be enqueued with the priority found on the message.
   *
   * \param[in] msg the message
   * \param[in] action the action to execute later
   */
  template <typename MsgT>
  void enqueue(MsgT* msg, ActionType action);

  /**
   * \brief Enqueue a action associated with a prioritized message. The action
   * will be enqueued with the priority found on the message.
   *
   * \param[in] msg the message
   * \param[in] action the action to execute later
   */
  template <typename MsgT>
  void enqueue(MsgSharedPtr<MsgT> msg, ActionType action);

  /**
   * \brief Get the work queue size
   *
   * \return how many units in the queue
   */
  std::size_t workQueueSize() const { return work_queue_.size(); }

  /**
   * \brief Query if the work queue is empty
   *
   * \return whether it is empty
   */
  bool workQueueEmpty() const { return work_queue_.empty(); }

  /**
   * \brief Check if the scheduler is idle
   *
   * \return whether this scheduler is idle
   */
  bool isIdle() const { return work_queue_.empty(); }

  /**
   * \internal \brief Check if the scheduler is idle minus termination messages
   *
   * \return whether this scheduler is idle
   */
  bool isIdleMinusTerm() const { return work_queue_.size() == num_term_msgs_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | work_queue_
      | has_executed_
      | is_idle
      | is_idle_minus_term
      | action_depth_
      | num_term_msgs_
      | event_triggers
      | event_triggers_once
      | last_progress_time_
      | progress_time_enabled_
      | processed_after_last_progress_
      | last_threshold_memory_usage_
      | threshold_memory_usage_
      | last_memory_usage_poll_;
  }

private:

  /**
   * \internal \brief Executes a specific work unit.
   *
   * \param[in] work work unit to execute
   */
  void runWorkUnit(UnitType& work);

  /**
   * \internal \brief Make progress on active message only
   *
   * \return whether progress was made
   */
  bool progressMsgOnlyImpl();

  /**
   * \internal \brief Make progress
   *
   * \return whether progress was made
   */
  bool progressImpl();

private:

# if vt_check_enabled(priorities)
  PriorityQueue<UnitType> work_queue_;
# else
  Queue<UnitType> work_queue_;
# endif

  bool has_executed_      = false;
  bool is_idle            = true;
  bool is_idle_minus_term = true;
  // The depth of work action currently executing.
  unsigned int action_depth_ = 0;

  // The number of termination messages currently in the queue---they weakly
  // imply idleness for the stake of termination
  std::size_t num_term_msgs_ = 0;

  EventTriggerContType event_triggers;
  EventTriggerContType event_triggers_once;

  TimeType last_progress_time_ = 0.0;
  bool progress_time_enabled_ = false;
  int32_t processed_after_last_progress_ = 0;

  std::size_t last_threshold_memory_usage_ = 0;
  std::size_t threshold_memory_usage_ = 0;
  std::size_t last_memory_usage_poll_ = 0;

  // Access to triggerEvent.
  template <typename Callable>
  friend void vt::runInEpochRooted(Callable&& fn);

  template <typename Callable>
  friend void vt::runInEpochCollective(Callable&& fn);

private:
  diagnostic::Counter progressCount;
  diagnostic::Counter workUnitCount;
  diagnostic::Gauge queueSizeGauge;
  diagnostic::Timer vtLiveTime;
  diagnostic::Timer schedLoopTime;
  diagnostic::Timer idleTime;
  diagnostic::Timer idleTimeMinusTerm;
};

}} //end namespace vt::sched

namespace vt {

extern sched::Scheduler* theSched();

}  //end namespace vt

#include "vt/scheduler/scheduler.impl.h"

#endif /*INCLUDED_SCHEDULER_SCHEDULER_H*/
