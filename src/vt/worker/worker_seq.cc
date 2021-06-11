/*
//@HEADER
// *****************************************************************************
//
//                                worker_seq.cc
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

#if vt_check_enabled(fcontext)

#include "vt/context/context.h"
#include "vt/context/context_attorney.h"
#include "vt/collective/collective_ops.h"
#include "vt/worker/worker_common.h"
#include "vt/worker/worker_seq.h"
#include "vt/utils/bits/bits_common.h"

#include <memory>
#include <functional>

#define WORKER_SEQ_VERBOSE 0

namespace vt { namespace worker {

WorkerSeq::WorkerSeq(
  WorkerIDType const& in_worker_id_, WorkerCountType const& in_num_thds,
  WorkerFinishedFnType finished_fn
) : worker_id_(in_worker_id_), num_thds_(in_num_thds), finished_fn_(finished_fn),
    stack(create_fcontext_stack())
{ }

void WorkerSeq::enqueue(WorkUnitType const& work_unit) {
  work_queue_.pushBack(work_unit);
}

void WorkerSeq::progress() {
  #if WORKER_SEQ_VERBOSE
  vt_debug_print(normal, worker, "WorkerSeq: progress: id={}\n", worker_id_);
  #endif

  auto new_live = jump_fcontext(live.ctx);
  live = new_live;
}

/*static*/ void WorkerSeq::workerSeqSched(fcontext_transfer_t t) {
  using ::vt::ctx::ContextAttorney;

  void* data = t.data;
  WorkerSeq* seq = reinterpret_cast<WorkerSeq*>(data);
  fcontext_transfer_t cur = t;

  #if WORKER_SEQ_VERBOSE
  vt_debug_print(normal, worker, "workerSeqSched: id={}\n", seq->worker_id_);
  #endif

  while (!seq->should_terminate_) {
    ContextAttorney::setWorker(seq->worker_id_);

    if (seq->work_queue_.size() > 0) {
      auto elm = seq->work_queue_.popGetBack();
      elm();
      seq->finished_fn_(seq->worker_id_, 1);
    }

    #if WORKER_SEQ_VERBOSE
    vt_debug_print(normal, worker, "seq sched: id={} jump out\n", seq->worker_id_);
    #endif

    fcontext_transfer_t new_ctx = jump_fcontext(cur.ctx, nullptr);
    cur = new_ctx;

    #if WORKER_SEQ_VERBOSE
    vt_debug_print(normal, worker, "seq sched: id={} jump in\n", seq->worker_id_);
    #endif
  }

  vt_debug_print(normal, worker, "seq sched: id={} term\n", seq->worker_id_);
  jump_fcontext(cur.ctx, nullptr);
}

void WorkerSeq::startScheduler() {
  vt_debug_print(normal, worker, "WorkerSeq: startScheduler: id={}\n", worker_id_);

  using ::vt::ctx::ContextAttorney;

  // Set the thread-local worker in the Context
  ContextAttorney::setWorker(worker_id_);

  fctx = make_fcontext_stack(stack, workerSeqSched);

  #if WORKER_SEQ_VERBOSE
  vt_debug_print(normal, worker, "WorkerSeq: jump context: id={}\n", worker_id_);
  #endif

  live = jump_fcontext(fctx, this);

  #if WORKER_SEQ_VERBOSE
  vt_debug_print(normal, worker, "WorkerSeq: jump context out: id={}\n", worker_id_);
  #endif
}

void WorkerSeq::sendTerminateSignal() {
  should_terminate_ = true;
}

void WorkerSeq::spawn() {
  vt_debug_print(normal, worker, "WorkerSeq: spawn: id={}\n", worker_id_);
  startScheduler();
}

void WorkerSeq::join() {
  vt_debug_print(normal, worker, "WorkerSeq: join: id={}\n", worker_id_);
  // tell the worker to return from the scheduler loop
  sendTerminateSignal();
  progress();
}

void WorkerSeq::dispatch(WorkerFunType fun) {
  enqueue(fun);
}

}} /* end namespace vt::worker */

#endif /*backend_no_threading*/

