/*
//@HEADER
// ************************************************************************
//
//                          worker_seq.h
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

#if !defined INCLUDED_WORKER_WORKER_SEQ_H
#define INCLUDED_WORKER_WORKER_SEQ_H

#include "vt/config.h"

#if backend_no_threading

#include "vt/worker/worker_common.h"
#include "vt/worker/worker_types.h"
#include "vt/utils/container/concurrent_deque.h"

#include <fcontext.h>

#include <functional>

namespace vt { namespace worker {

struct WorkerSeq {
  using WorkerFunType = std::function<void()>;
  using WorkUnitContainerType = util::container::ConcurrentDeque<WorkUnitType>;

  WorkerSeq(
    WorkerIDType const& in_worker_id_, WorkerCountType const& in_num_thds,
    WorkerFinishedFnType finished_fn
  );
  WorkerSeq(WorkerSeq const&) = delete;

  void spawn();
  void join();
  void dispatch(WorkerFunType fun);
  void enqueue(WorkUnitType const& work_unit);
  void sendTerminateSignal();
  void progress();

private:
  void startScheduler();
  void continueScheduler();

  static void workerSeqSched(fcontext_transfer_t t);

private:
  bool should_terminate_ = false;
  WorkerIDType worker_id_ = no_worker_id;
  WorkerCountType num_thds_ = no_workers;
  WorkUnitContainerType work_queue_;
  WorkerFinishedFnType finished_fn_ = nullptr;
  fcontext_stack_t stack;
  fcontext_t fctx;
  fcontext_transfer_t live;
};

}} /* end namespace vt::worker */

#if backend_check_enabled(detector)
  #include "vt/worker/worker_traits.h"

  namespace vt { namespace worker {

  static_assert(
    WorkerTraits<WorkerSeq>::is_worker,
    "vt::worker::Worker must follow the Worker concept"
  );

  }} /* end namespace vt::worker */
#endif /*backend_check_enabled(detector)*/

#endif /*backend_no_threading*/

#endif /*INCLUDED_WORKER_WORKER_SEQ_H*/
