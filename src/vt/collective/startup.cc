/*
//@HEADER
// *****************************************************************************
//
//                                  startup.cc
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
#include "vt/collective/startup.h"
#include "vt/collective/collective_ops.h"
#include "vt/runtime/runtime_headers.h"
#include "vt/context/context.h"

namespace vt {

// vt::{initialize,finalize} for main ::vt namespace
RuntimePtrType initialize(
  int& argc, char**& argv, WorkerCountType const num_workers,
  bool is_interop, MPI_Comm* comm
) {
  return CollectiveOps::initialize(argc,argv,num_workers,is_interop,comm);
}

RuntimePtrType initialize(int& argc, char**& argv, MPI_Comm* comm) {
  bool const is_interop = comm != nullptr;
  return CollectiveOps::initialize(argc,argv,no_workers,is_interop,comm);
}

RuntimePtrType initialize(MPI_Comm* comm) {
  int argc = 0;
  char** argv = nullptr;
  return CollectiveOps::initialize(argc,argv,no_workers,true,comm);
}

RuntimePtrType allocate(
  bool is_interop,
  WorkerCountType const num_workers,
  MPI_Comm* comm
) {
  return CollectiveOps::allocate(is_interop, num_workers, comm);
}

void finalize(RuntimePtrType in_rt) {
  if (in_rt) {
    return CollectiveOps::finalize(std::move(in_rt));
  } else {
    return CollectiveOps::finalize(nullptr);
  }
}

void finalize() {
  CollectiveOps::finalize(nullptr);
}

} /* end namespace vt */
