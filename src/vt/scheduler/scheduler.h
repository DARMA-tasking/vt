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
#include "vt/trace/trace.h"

#include <cassert>
#include <vector>
#include <list>
#include <functional>
#include <memory>

namespace vt { namespace sched {

enum SchedulerEvent {
  BeginIdle          = 0,
  EndIdle            = 1,
  BeginIdleMinusTerm = 2,
  EndIdleMinusTerm   = 3,
  SchedulerEventSize = 4
};

/**
 * \brief The type of the blocking scheduler loop being run.
 *
 * Values less than 1024 are reserved for internal VT usages.
 */
enum struct SchedulerTypeID : uint32_t {
  // Core
  BarrierWait = 100,
  // Termination
  TermRooted = 200,
  TermCollective = 201,
  TermEpochGraph = 202,
  // VRT / Collection
  CollectionConstruct = 300,
  // Load Balancing
  LbWaitCollective = 400,
  LbGossipInform = 401,
  LbGossipDecide = 402,
  // Misc. support
  TraceSpecLoading = 500
};

struct SchedulerRunner;

struct Scheduler {
  using SchedulerEventType   = SchedulerEvent;
  using TriggerType          = std::function<void()>;
  using TriggerContainerType = std::list<TriggerType>;
  using EventTriggerContType = std::vector<TriggerContainerType>;

# if backend_check_enabled(priorities)
  using UnitType             = PriorityUnit;
# else
  using UnitType             = Unit;
# endif

  Scheduler();

  static void checkTermSingleNode();

  bool shouldCallProgress(
    int32_t processed_since_last_progress, TimeType time_since_last_progress
  ) const;

  void scheduler(bool msg_only = false);
  void runProgress(bool msg_only = false);

  /**
   * \brief Obtain a scheduler runner.
   *
   * A scheduler that can be used for a local BLOCKING poll.
   * The returnted scheduler shares the same events as the global scheduler.
   *
   * This can be used in place of the the standard vt::runScheduler()
   * and SHOULD be used in all library code outside of the primary scheduling
   * loop (which can remain unchanged). In particular, using a scheduler
   * runner appropriately emits diagnotic tracing events as relevant.
   *
   * The scheduler uses RAII and should be destructed as soon
   * as the usage (eg. scheduler loop) is finished. The object should not
   * be moved or copied to different contexts.
   *
   * \param scheduler_tag - Tag value to write to trace data for additional context.
   *                        Values less than 1024 are reserved for internal VT usages.
   */
  SchedulerRunner beginScheduling(SchedulerTypeID scheduler_tag);

  void registerTrigger(SchedulerEventType const& event, TriggerType trigger);
  void registerTriggerOnce(
    SchedulerEventType const& event, TriggerType trigger
  );
  void triggerEvent(SchedulerEventType const& event);

  bool hasSchedRun() const { return has_executed_; }

  void enqueue(ActionType action);
  void enqueue(PriorityType priority, ActionType action);

  template <typename MsgT>
  void enqueue(MsgT* msg, ActionType action);
  template <typename MsgT>
  void enqueue(MsgSharedPtr<MsgT> msg, ActionType action);

  std::size_t workQueueSize() const { return work_queue_.size(); }
  bool workQueueEmpty() const { return work_queue_.empty(); }

  bool isIdle() const { return work_queue_.empty(); }
  bool isIdleMinusTerm() const { return work_queue_.size() == num_term_msgs_; }

private:

  /**
   * \brief Executes a specific work unit.
   *
   * Returns true if returning from the TOP LEVEL scheduler running.
   * (Nested schedulers can be run as a result of barriers, etc.)
   */
  bool runWorkUnit(UnitType& work);
  bool progressMsgOnlyImpl();
  bool progressImpl();

private:

# if backend_check_enabled(priorities)
  PriorityQueue<UnitType> work_queue_;
# else
  Queue<UnitType> work_queue_;
# endif

  bool has_executed_      = false;
  bool is_idle            = true;
  bool is_idle_minus_term = true;
  // The depth of work action currently executing.
  unsigned int unit_run_depth_ = 0;

  // The number of termination messages currently in the queue---they weakly
  // imply idleness for the stake of termination
  std::size_t num_term_msgs_ = 0;

  EventTriggerContType event_triggers;
  EventTriggerContType event_triggers_once;

  TimeType last_progress_time_ = 0.0;
  bool progress_time_enabled_ = false;
  int32_t processed_after_last_progress_ = 0;
};

/**
  * \brief Facilitates helping with nested scheduling contexts.
  *
  * This is a Resource Acquisition type that ensures pre/post
  * state cleanup when invoking a scheduler in a nested context.
  * It should be destructed immediately after scheduling.
  */
struct SchedulerRunner {
  SchedulerRunner(unsigned int in_sched_depth, SchedulerTypeID in_sched_tag);

  SchedulerRunner(SchedulerRunner const&) = default;
  SchedulerRunner& operator=(SchedulerRunner const&) = default;

  ~SchedulerRunner();

  /**
   * \brief Invokes one cycle of the scheduler.
   *
   * Returns false on VT termination.
   */
  bool runScheduler();

private:
  unsigned int sched_depth_;
  SchedulerTypeID sched_tag_;

#if backend_check_enabled(trace_enabled)
  trace::TraceProcessingTag processing_event_;
#endif
};

}} //end namespace vt::sched

#include "vt/scheduler/scheduler.impl.h"

namespace vt {

void runScheduler();

extern sched::Scheduler* theSched();

}  //end namespace vt

#endif /*INCLUDED_SCHEDULER_SCHEDULER_H*/
