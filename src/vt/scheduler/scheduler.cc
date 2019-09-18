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

  #if backend_check_enabled(trace_enabled)
    trace_start_scheduler = trace::registerEventCollective(
      "Scheduler::schedulerImpl (begin)"
    );
    trace_stop_scheduler = trace::registerEventCollective(
      "Scheduler::schedulerImpl (end)"
    );
  #endif
}

bool Scheduler::schedulerImpl() {
  #if backend_check_enabled(trace_enabled)
    trace::addUserEvent(trace_start_scheduler);
  #endif

  bool scheduled_work = false;

  bool const msg_sch = theMsg()->scheduler();
  bool const event_sch = theEvent()->scheduler();
  bool const seq_sch = theSeq()->scheduler();
  bool const vrt_seq_sch = theVirtualSeq()->scheduler();
  bool const collection_sch = theCollection()->scheduler<>();
  bool const objgroup_sch = objgroup::scheduler();
  bool const worker_sch =
    theContext()->hasWorkers() ? theWorkerGrp()->progress(),false : false;
  bool const worker_comm_sch =
    theContext()->hasWorkers() ? theWorkerGrp()->commScheduler() : false;

  checkTermSingleNode();

  scheduled_work =
    msg_sch or event_sch or seq_sch or vrt_seq_sch or
    worker_sch or worker_comm_sch or collection_sch or objgroup_sch;

  if (scheduled_work) {
    is_idle = false;
  }

  #if backend_check_enabled(trace_enabled)
    trace::addUserEvent(trace_stop_scheduler);
  #endif

  return scheduled_work;
}

void Scheduler::scheduler() {
  bool const scheduled_work1 = schedulerImpl();
  bool const scheduled_work2 = schedulerImpl();

  if (not scheduled_work1 and not scheduled_work2 and not is_idle) {
    is_idle = true;
    // idle
    triggerEvent(SchedulerEventType::BeginIdle);
  }

  has_executed_ = true;
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
