/*
//@HEADER
// *****************************************************************************
//
//                               worker_openmp.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if vt_check_enabled(openmp)

#include "vt/context/context.h"
#include "vt/worker/worker_common.h"
#include "vt/worker/worker_openmp.h"

#include <memory>
#include <functional>

#include <omp.h>

#define DEBUG_OMP_WORKER_SCHEDULER 0

namespace vt { namespace worker {

OMPWorker::OMPWorker(
  WorkerIDType const& in_worker_id_, WorkerIDType const& in_num_thds,
  WorkerFinishedFnType finished_fn
) : worker_id_(in_worker_id_), finished_fn_(finished_fn)
{ }

void OMPWorker::enqueue(WorkUnitType const& work_unit) {
  work_queue_.pushBack(work_unit);
}

void OMPWorker::progress() {
  // Noop
}

void OMPWorker::scheduler() {
  bool should_term_local = false;
  do {
    if (work_queue_.size() > 0) {
      #if DEBUG_OMP_WORKER_SCHEDULER
      vt_debug_print(
        normal, worker,
        "OMPWorker: scheduler: size={}\n", work_queue_.size()
      );
      #endif

      auto elm = work_queue_.popGetBack();
      elm();
      finished_fn_(worker_id_, 1);
    }

    #pragma omp atomic read
    should_term_local = should_terminate_;
  } while (not should_term_local);
}

void OMPWorker::sendTerminateSignal() {
  #pragma omp atomic write
  should_terminate_ = true;

  vt_debug_print(normal, worker, "OMPWorker: sendTerminateSignal\n");
}

void OMPWorker::spawn() {
  vt_debug_print(
    normal, worker,
    "OMPWorker: spawn: spawning worker: id={}\n", worker_id_
  );

  scheduler();
}

void OMPWorker::join() {
  vt_debug_print(
    normal, worker,
    "OMPWorker: join: spawning worker: id={}\n", worker_id_
  );

  // tell the worker to return from the scheduler loop
  sendTerminateSignal();
}

void OMPWorker::dispatch(WorkerFunType fun) {
  enqueue(fun);
}

}} /* end namespace vt::worker */

#endif /*vt_check_enabled(openmp)*/
