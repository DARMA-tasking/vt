/*
//@HEADER
// *****************************************************************************
//
//                             worker_group_omp.cc
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
#include "vt/context/context_attorney.h"
#include "vt/collective/collective_ops.h"

#if backend_check_enabled(openmp)

#include "vt/worker/worker_common.h"
#include "vt/worker/worker_group_omp.h"

#include <functional>

#include <omp.h>

#define WORKER_OMP_VERBOSE 0

namespace vt { namespace worker {

WorkerGroupOMP::WorkerGroupOMP()
  : WorkerGroupOMP(num_default_workers)
{ }

WorkerGroupOMP::WorkerGroupOMP(WorkerCountType const& in_num_workers)
  : num_workers_(in_num_workers)
{
  initialize();
}

void WorkerGroupOMP::initialize() {
  namespace ph = std::placeholders;
  finished_fn_ = std::bind(&WorkerGroupOMP::finished, this, ph::_1, ph::_2);

  worker_state_.resize(num_workers_);
}

bool WorkerGroupOMP::commScheduler() {
  return WorkerGroupComm::schedulerComm(finished_fn_);
}

void WorkerGroupOMP::progress() {
  WorkerGroupCounter::progress();
}

/*virtual*/ WorkerGroupOMP::~WorkerGroupOMP() {
  worker_state_.clear();
}

void WorkerGroupOMP::enqueueCommThread(WorkUnitType const& work_unit) {
  // if (theContext()->getWorker() != worker_id_comm_thread) {
  //   enqueue_worker_mutex_.lock();
  // }
  this->enqueued();
  WorkerGroupComm::enqueueComm(work_unit);
  // if (theContext()->getWorker() != worker_id_comm_thread) {
  //   enqueue_worker_mutex_.unlock();
  // }
}

void WorkerGroupOMP::spawnWorkersBlock(WorkerCommFnType comm_fn) {
  using ::vt::ctx::ContextAttorney;

  debug_print(
    worker, node,
    "Worker group OMP: launching num worker threads={}, num comm threads={}\n",
    num_workers_, num_default_comm
  );

  initialized_ = true;

  debug_print(
    worker, node,
    "worker group OMP spawning={}\n", num_workers_ + 1
  );

  #pragma omp parallel num_threads(num_workers_ + 1)
  {
    WorkerIDType const thd = omp_get_thread_num();
    WorkerIDType const nthds = omp_get_num_threads();

    // For now, all workers to have direct access to the runtime
    // TODO: this needs to change
    CollectiveOps::setCurrentRuntimeTLS();

    if (thd < num_workers_) {
      // Set the thread-local worker in Context
      ContextAttorney::setWorker(thd);

      debug_print(
        worker, node,
        "Worker group OMP: (worker) thd={}, num threads={}\n", thd, nthds
      );

      worker_state_[thd] = std::make_unique<WorkerStateType>(
        thd, nthds, finished_fn_
      );
      ready_++;
      worker_state_[thd]->spawn();
    } else {
      // Set the thread-local worker in Context
      ContextAttorney::setWorker(worker_id_comm_thread);

      debug_print(
        worker, node,
        "Worker group OMP: (comm) thd={}, num threads={}\n", thd, nthds
      );

      // Wait until all the workers are created and have filled the
      // worker_state_ vector
      while (ready_.load() < num_workers_) ;

      // Enqueue an initial work unit for termination purposes
      auto initial_work_unit = []{};
      enqueueAllWorkers(initial_work_unit);

      // launch comm function on the main communication thread
      comm_fn();

      // once the comm function exits the program is terminated
      for (auto thr = 0; thr < num_workers_; thr++) {
        debug_print( worker, node, "comm: calling join thd={}\n", thr );
        worker_state_[thr]->join();
      }
    }
  }
}

void WorkerGroupOMP::spawnWorkers() {
  vtAssert(0, "Not supported on OMP workers");
}

void WorkerGroupOMP::joinWorkers() {
  for (int i = 0; i < num_workers_; i++) {
    worker_state_[i]->sendTerminateSignal();
  }
}

void WorkerGroupOMP::enqueueAnyWorker(WorkUnitType const& work_unit) {
  vtAssert(initialized_, "Must be initialized to enqueue");

  #if WORKER_OMP_VERBOSE
  debug_print(worker, node, "WorkerGroupOMP: enqueue any worker\n");
  #endif

  this->enqueued();
  worker_state_[0]->enqueue(work_unit);
}

void WorkerGroupOMP::enqueueForWorker(
  WorkerIDType const& worker_id, WorkUnitType const& work_unit
) {
  vtAssert(initialized_, "Must be initialized to enqueue");
  vtAssert(
    static_cast<size_t>(worker_id) < worker_state_.size(),
    "Worker ID must be valid"
  );

  #if WORKER_OMP_VERBOSE
  debug_print(worker, node, "WorkerGroupOMP: enqueue for id={}\n", worker_id);
  #endif

  this->enqueued();
  worker_state_[worker_id]->enqueue(work_unit);
}

void WorkerGroupOMP::enqueueAllWorkers(WorkUnitType const& work_unit) {
  vtAssert(initialized_, "Must be initialized to enqueue");

  #if WORKER_OMP_VERBOSE
  debug_print(worker, node, "WorkerGroupOMP: enqueue all workers\n");
  #endif

  this->enqueued(num_workers_);
  for (auto&& elm : worker_state_) {
    elm->enqueue(work_unit);
  }
}

}} /* end namespace vt::worker */

#endif
