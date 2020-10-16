/*
//@HEADER
// *****************************************************************************
//
//                              request_holder.cc
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

#if !defined INCLUDED_VT_RDMAHANDLE_REQUEST_HOLDER_CC
#define INCLUDED_VT_RDMAHANDLE_REQUEST_HOLDER_CC

#include "vt/config.h"
#include "vt/rdmahandle/request_holder.h"
#include "vt/runtime/mpi_access.h"

namespace vt { namespace rdma {

void RequestHolder::add(std::function<void()> fn) {
  delayed_ = fn;
}

MPI_Request* RequestHolder::add() {
  reqs_.emplace_back(MPI_Request{});
  return &reqs_[reqs_.size()-1];
}

bool RequestHolder::test() {
  VT_ALLOW_MPI_CALLS;
  std::vector<MPI_Request> new_reqs;
  std::vector<MPI_Status> stats;
  stats.resize(reqs_.size());
  auto flags = std::make_unique<int[]>(reqs_.size());
  MPI_Testall(reqs_.size(), &reqs_[0], &flags[0], &stats[0]);
  for (std::size_t i = 0; i < reqs_.size(); i++) {
    if (not flags[i]) {
      new_reqs.push_back(reqs_[i]);
    }
  }
  reqs_ = std::move(new_reqs);
  return new_reqs.size() == 0;
}

void RequestHolder::wait() {
  vt_debug_print(
    rdma, node,
    "RequestHolder::wait: len={}, ptr={}\n",
    reqs_.size(), on_done_ ? "yes" : "no"
  );

  if (delayed_ != nullptr) {
    delayed_();
    delayed_ = nullptr;
  }

  theSched()->runSchedulerWhile([this]{ return not test(); });

  if (on_done_ != nullptr) {
    on_done_->invoke();
  }
  on_done_ = nullptr;
}

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_REQUEST_HOLDER_CC*/
