/*
//@HEADER
// *****************************************************************************
//
//                                  context.h
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

