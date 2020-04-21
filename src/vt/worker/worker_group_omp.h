/*
//@HEADER
// *****************************************************************************
//
//                              worker_group_omp.h
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

#if !defined INCLUDED_WORKER_WORKER_GROUP_OMP_H
#define INCLUDED_WORKER_WORKER_GROUP_OMP_H

#include "vt/config.h"

#if backend_check_enabled(openmp)

#include "vt/worker/worker_common.h"
#include "vt/worker/worker_types.h"
#include "vt/worker/worker_openmp.h"
#include "vt/worker/worker_group_counter.h"
#include "vt/worker/worker_group_comm.h"
#include "vt/utils/mutex/mutex.h"
#include "vt/runtime/component/component_pack.h"

#include <vector>
#include <memory>
#include <functional>

namespace vt { namespace worker {

struct WorkerGroupOMP
  : runtime::component::PollableComponent<WorkerGroupOMP>,
    WorkerGroupCounter, WorkerGroupComm
{
  using WorkerType = OMPWorker;
  using WorkerStateType = WorkerType;
  using WorkerStatePtrType = std::unique_ptr<WorkerStateType>;
  using WorkerStateContainerType = std::vector<WorkerStatePtrType>;
  using WorkerFunType = std::function<void()>;
  using WorkUnitContainerType = util::container::ConcurrentDeque<WorkUnitType>;
  using MutexType = util::mutex::MutexType;

  WorkerGroupOMP();
  WorkerGroupOMP(WorkerCountType const& in_num_workers);

  virtual ~WorkerGroupOMP();

  std::string name() override { return "WorkerGroupOMP"; }

  void initialize() override;
  void spawnWorkers();
  void spawnWorkersBlock(WorkerCommFnType fn);
  void joinWorkers();
  int progress() override;
  bool commScheduler();

  // non-thread-safe comm and worker thread enqueue
  void enqueueCommThread(WorkUnitType const& work_unit);
  void enqueueAnyWorker(WorkUnitType const& work_unit);
  void enqueueForWorker(
    WorkerIDType const& worker_id, WorkUnitType const& work_unit
  );
  void enqueueAllWorkers(WorkUnitType const& work_unit);

private:
  WorkerFinishedFnType finished_fn_ = nullptr;
  AtomicType<WorkerCountType> ready_ = {0};
  bool initialized_ = false;
  WorkerCountType num_workers_ = 0;
  WorkerStateContainerType worker_state_;
  MutexType enqueue_worker_mutex_{};
};

}} /* end namespace vt::worker */

#if backend_check_enabled(detector)
  #include "vt/worker/worker_group_traits.h"

  namespace vt { namespace worker {

  static_assert(
    WorkerGroupTraits<WorkerGroupOMP>::is_worker,
    "WorkerGroupOMP must follow the WorkerGroup concept"
  );

  }} /* end namespace vt::worker */
#endif /*backend_check_enabled(detector)*/

#endif /*backend_check_enabled(openmp)*/

#endif /*INCLUDED_WORKER_WORKER_GROUP_OMP_H*/
