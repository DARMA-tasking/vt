/*
//@HEADER
// *****************************************************************************
//
//                           worker_group_counter.cc
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
#include "vt/context/context.h"
#include "vt/worker/worker_common.h"
#include "vt/worker/worker_group_counter.h"
#include "vt/termination/term_headers.h"

#include <cassert>
#include <functional>

#define WORKER_COUNTER_VERBOSE 0

namespace vt { namespace worker {

void WorkerGroupCounter::attachEnqueueProgressFn() {
  namespace ph = std::placeholders;
  auto fn = std::bind(&WorkerGroupCounter::enqueued, this, ph::_1);
  enqueued_count_.attach(fn, worker_id_comm_thread);
}

void WorkerGroupCounter::enqueued(WorkUnitCountType num) {
  auto const is_comm = theContext()->getWorker() == worker_id_comm_thread;
  if (is_comm) {
    enqueuedComm(num);
  } else {
    enqueued_count_.push(num);
  }
}

void WorkerGroupCounter::enqueuedComm(WorkUnitCountType num) {
  assertCommThread();

  theTerm()->produce(term::any_epoch_sentinel, num);

  num_enqueued_ += num;
  maybe_idle_.store(false);

  // Call progress to update in the case where state was previously reported as
  // idle to the listeners
  progress();
}

void WorkerGroupCounter::finished(WorkerIDType id, WorkUnitCountType num) {
  #if WORKER_COUNTER_VERBOSE
  vt_debug_print(
    worker, node,
    "WorkerGroupCounter: completed: id={}, num={}\n", id, num
  );
  #endif

  // This method may be called from multiple threads
  auto const cur_finished = num_finished_.fetch_add(num) + num;
  auto const cur_enqueued = num_enqueued_.load();
  bool const is_idle = cur_finished == cur_enqueued;
  if (is_idle) {
    maybe_idle_.store(is_idle);
  }
}

void WorkerGroupCounter::assertCommThread() {
  // Sanity check to ensure single-threaded methods are only called by the
  // communication thread
  vtAssert(
    theContext()->getWorker() == worker_id_comm_thread,
    "This must only run on the communication thread"
  );
}

void WorkerGroupCounter::registerIdleListener(IdleListenerType listener) {
  assertCommThread();
  listeners_.push_back(listener);
}

void WorkerGroupCounter::progress() {
  bool const cur_maybe_idle = maybe_idle_.load();
  bool const last_event_idle = last_event_ == eWorkerGroupEvent::WorkersIdle;

  assertCommThread();

  enqueued_count_.progress();

  if (cur_maybe_idle || last_event_idle) {
    auto const cur_finished = num_finished_.load();
    auto const cur_enqueued = num_enqueued_.load();
    bool const is_idle = cur_finished == cur_enqueued;

    #if WORKER_COUNTER_VERBOSE
    vt_debug_print(
      worker, node,
      "WorkerGroupCounter: progress: fin={}, enq={}, is_idle={}, "
      "last_event={}, last_event_idle={}\n",
      cur_finished, cur_enqueued, print_bool(is_idle),
      WORKER_GROUP_EVENT_STR(last_event_), print_bool(last_event_idle)
    );
    #endif

    if (is_idle && !last_event_idle) {
      // trigger listeners
      triggerListeners(eWorkerGroupEvent::WorkersIdle);
    } else if (!is_idle) {
      if (last_event_idle) {
        triggerListeners(eWorkerGroupEvent::WorkersBusy);
      }
      maybe_idle_.store(is_idle);
    } else if (is_idle && num_finished_.load() > num_consumed_) {
      updateConsumedTerm();
    }
  }
}

void WorkerGroupCounter::updateConsumedTerm() {
  auto const cur_finished = num_finished_.load();
  auto const cur_consumed = num_consumed_;
  auto const num_to_consume = cur_finished - cur_consumed;

  num_consumed_ += num_to_consume;

  vt_debug_print(
    worker, node,
    "WorkerGroupCounter: updating: num_fin={}, num_con={}, num_sub={}\n",
    cur_finished, cur_consumed, num_to_consume
  );

  theTerm()->consume(term::any_epoch_sentinel, num_to_consume);
}

void WorkerGroupCounter::triggerListeners(eWorkerGroupEvent event) {
  #if WORKER_COUNTER_VERBOSE
  vt_debug_print(
    worker, node,
    "WorkerGroupCounter: triggering listeners: event={}\n",
    WORKER_GROUP_EVENT_STR(event)
  );
  #endif

  if (event == eWorkerGroupEvent::WorkersIdle) {
    updateConsumedTerm();
  }

  last_event_ = event;

  for (auto&& elm : listeners_) {
    elm(event);
  }
}

}} /* end namespace vt::worker */
