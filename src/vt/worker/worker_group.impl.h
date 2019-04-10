/*
//@HEADER
// ************************************************************************
//
//                          worker_group.impl.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_WORKER_WORKER_GROUP_IMPL_H
#define INCLUDED_WORKER_WORKER_GROUP_IMPL_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/worker/worker_common.h"
#include "vt/worker/worker_group.h"

#include <functional>
#include <cstdint>

namespace vt { namespace worker {

template <typename WorkerT>
WorkerGroupAny<WorkerT>::WorkerGroupAny()
  : WorkerGroupAny(num_default_workers)
{ }

template <typename WorkerT>
WorkerGroupAny<WorkerT>::WorkerGroupAny(WorkerCountType const& in_num_workers)
  : num_workers_(in_num_workers)
{
  debug_print(
    worker, node,
    "WorkerGroup: constructing with default: num_workers_={}\n",
    num_workers_
  );

  initialize();
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::initialize() {
  namespace ph = std::placeholders;
  finished_fn_ = std::bind(&WorkerGroupAny::finished, this, ph::_1, ph::_2);

  workers_.resize(num_workers_);
}

template <typename WorkerT>
bool WorkerGroupAny<WorkerT>::commScheduler() {
  return WorkerGroupComm::schedulerComm(finished_fn_);
}

template <typename WorkerT>
/*virtual*/ WorkerGroupAny<WorkerT>::~WorkerGroupAny() {
  if (workers_.size() > 0) {
    joinWorkers();
  }
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueCommThread(WorkUnitType const& work_unit) {
  vtAssert(initialized_, "Must be initialized to enqueue");
  this->enqueued();
  WorkerGroupComm::enqueueComm(work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueAnyWorker(WorkUnitType const& work_unit) {
  vtAssert(initialized_, "Must be initialized to enqueue");

  this->enqueued();
  workers_[0].enqueue(work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueForWorker(
  WorkerIDType const& worker_id, WorkUnitType const& work_unit
) {
  vtAssert(initialized_, "Must be initialized to enqueue");
  vtAssert((size_t)worker_id < workers_.size(), "Worker ID must be valid");

  this->enqueued();
  workers_[worker_id]->enqueue(work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueAllWorkers(WorkUnitType const& work_unit) {
  vtAssert(initialized_, "Must be initialized to enqueue");

  this->enqueued(num_workers_);

  for (auto&& elm : workers_) {
    elm->enqueue(work_unit);
  }
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::progress() {
  for (auto&& elm : workers_) {
    elm->progress();
  }

  WorkerGroupCounter::progress();
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::spawnWorkersBlock(WorkerCommFnType comm_fn) {
  debug_print(
    worker, node,
    "WorkerGroup: spawnWorkersBlock: num_workers_={}\n", num_workers_
  );

  // spawn the workers
  spawnWorkers();

  // block the comm thread on the passed function
  comm_fn();
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::spawnWorkers() {
  using namespace std::placeholders;

  debug_print(
    worker, node,
    "WorkerGroup: spawnWorkers: num_workers_={}\n", num_workers_
  );

  vtAssert(workers_.size() >= (size_t)num_workers_, "Must be correct size");

  for (int i = 0; i < num_workers_; i++) {
    WorkerIDType const worker_id = i;
    workers_[i] = std::make_unique<WorkerT>(
      worker_id, num_workers_, finished_fn_
    );
  }

  for (auto&& elm : workers_) {
    elm->spawn();
  }

  initialized_ = true;

  // Give every worker at least one work unit (for termination purposes)
  auto initial_work_unit = []{};
  enqueueAllWorkers(initial_work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::joinWorkers() {
  debug_print(
    worker, node,
    "WorkerGroup: joinWorkers: num_workers_={}\n", num_workers_
  );

  for (auto&& elm : workers_) {
    elm->join();
  }

  workers_.clear();

  initialized_ = false;
}

}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_GROUP_IMPL_H*/
