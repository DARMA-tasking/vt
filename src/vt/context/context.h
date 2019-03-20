/*
//@HEADER
// ************************************************************************
//
//                          context.h
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

#if !defined INCLUDED_CONTEXT
#define INCLUDED_CONTEXT

#include <memory>
#include <mpi.h>

#include "vt/config.h"
#include "vt/context/context_attorney_fwd.h"
#include "vt/utils/tls/tls.h"

namespace vt {  namespace ctx {

struct Context {
  Context(int argc, char** argv, bool const interop, MPI_Comm* comm = nullptr);
  Context(bool const interop, MPI_Comm* comm = nullptr);

  inline NodeType getNode() const { return thisNode_; }
  inline NodeType getNumNodes() const { return numNodes_; }

  inline MPI_Comm getComm() const { return communicator_; }
  inline bool isCommWorld() const { return is_comm_world_; }

  inline WorkerCountType getNumWorkers() const { return numWorkers_; }
  inline bool hasWorkers() const { return numWorkers_ != no_workers; }
  inline WorkerIDType getWorker() const {
    return AccessClassTLS(Context, thisWorker_);
  }

  friend struct ContextAttorney;

protected:
  void setNumWorkers(WorkerCountType const worker_count) {
    numWorkers_ = worker_count;
  }
  void setWorker(WorkerIDType const worker) {
    AccessClassTLS(Context, thisWorker_) = worker;
  }

private:
  void setDefaultWorker();

private:
  NodeType thisNode_ = uninitialized_destination;
  NodeType numNodes_ = uninitialized_destination;
  WorkerCountType numWorkers_ = no_workers;
  bool is_comm_world_ = true;
  MPI_Comm communicator_ = MPI_COMM_WORLD;
  DeclareClassInsideInitTLS(Context, WorkerIDType, thisWorker_, no_worker_id)
};

}} // end namespace vt::ctx

namespace vt {

extern ctx::Context* theContext();

} // end namespace vt

#endif /*INCLUDED_CONTEXT*/

