/*
//@HEADER
// *****************************************************************************
//
//                             worker_group_comm.cc
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
#include "vt/worker/worker_group_comm.h"

namespace vt { namespace worker {

void WorkerGroupComm::enqueueComm(WorkUnitType const& work_unit) {
  vt_debug_print(
    normal, worker,
    "WorkerGroupComm: enqueue comm thread size={}\n", comm_work_deque_.size()
  );

  comm_work_deque_.pushBack(work_unit);
}

bool WorkerGroupComm::schedulerComm(WorkerFinishedFnType finished_fn) {
  bool found = false;
  if (comm_work_deque_.size() > 0) {
    vt_debug_print(
      normal, worker,
      "WorkerGroupComm: scheduler executing size={}\n", comm_work_deque_.size()
    );

    auto work_unit = comm_work_deque_.front();
    comm_work_deque_.popFront();
    work_unit();
    found = true;

    if (finished_fn) {
      finished_fn(worker_id_comm_thread, 1);
    }
  }
  return found;
}

}} /* end namespace vt::worker */
