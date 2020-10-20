/*
//@HEADER
// *****************************************************************************
//
//                                  lock_mpi.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_LOCK_MPI_H
#define INCLUDED_VT_RDMAHANDLE_LOCK_MPI_H

#include "vt/config.h"
#include "vt/runtime/mpi_access.h"

namespace vt { namespace rdma {

enum struct Lock : int8_t {
  None      = 0,
  Exclusive = 1,
  Shared    = 2
};

struct LockMPI {
  LockMPI(Lock in_l, vt::NodeType in_rank, MPI_Win in_window)
    : l_(in_l),
      rank_(in_rank),
      window_(in_window)
  {
    if (l_ != Lock::None) {
      auto lock_type = l_ == Lock::Exclusive ? MPI_LOCK_EXCLUSIVE : MPI_LOCK_SHARED;
      VT_ALLOW_MPI_CALLS;
      MPI_Win_lock(lock_type, rank_, 0, window_);
    }
  }
  LockMPI(LockMPI const&) = delete;
  LockMPI(LockMPI&&) = default;

  ~LockMPI() {
    if (l_ != Lock::None) {
      VT_ALLOW_MPI_CALLS;
      MPI_Win_unlock(rank_, window_);
    }
  }

private:
  Lock l_ = Lock::None;
  vt::NodeType rank_ = vt::uninitialized_destination;
  MPI_Win window_;
};

}} /* end namespace vt::rdma */

namespace vt {

using Lock = rdma::Lock;

} /* end namespace vt */

#endif /*INCLUDED_VT_RDMAHANDLE_LOCK_MPI_H*/
