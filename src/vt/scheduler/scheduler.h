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
#include "vt/scheduler/work_unit.h"

#include <cassert>
#include <vector>
#include <list>
#include <functional>
#include <memory>

namespace vt { namespace sched {

enum SchedulerEvent {
  BeginIdle = 0,
  EndIdle = 1,
  SchedulerEventSize = 2
};

struct Scheduler {
  using SchedulerEventType = SchedulerEvent;
  using TriggerType = std::function<void()>;
  using TriggerContainerType = std::list<TriggerType>;
  using EventTriggerContType = std::vector<TriggerContainerType>;

  Scheduler();

  static void checkTermSingleNode();

  bool runNextUnit();
  bool progressMsgOnlyImpl();
  void scheduler(bool msg_only = false);
  bool progressImpl();
  void schedulerForever();
  void registerTrigger(SchedulerEventType const& event, TriggerType trigger);
  void registerTriggerOnce(
    SchedulerEventType const& event, TriggerType trigger
  );
  void triggerEvent(SchedulerEventType const& event);
  bool hasSchedRun() const { return has_executed_; }

  void enqueue(ActionType action);
  std::size_t workQueueSize() const { return work_queue_.size(); }
  bool workQueueEmpty() const { return work_queue_.empty(); }

private:
  Queue<Unit> work_queue_;

  bool has_executed_ = false;
  bool is_idle = false;

  EventTriggerContType event_triggers;
  EventTriggerContType event_triggers_once;
};

}} //end namespace vt::scheduler

namespace vt {

void runScheduler();

extern sched::Scheduler* theSched();

}  //end namespace vt

#endif /*INCLUDED_SCHEDULER_SCHEDULER_H*/
