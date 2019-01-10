/*
//@HEADER
// ************************************************************************
//
//                          collective_ops.h
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

#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_OPS_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_OPS_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/runtime/runtime_headers.h"
#include "vt/registry/registry.h"

#include <string>

#include <mpi.h>

namespace vt {

using namespace ::vt::runtime;

static constexpr RuntimeInstType const collective_default_inst =
  RuntimeInstType::DefaultInstance;

template <RuntimeInstType instance = collective_default_inst>
struct CollectiveAnyOps {
  // The general methods that interact with the managed runtime holder
  static RuntimePtrType initialize(
    int& argc, char**& argv, WorkerCountType const num_workers = no_workers,
    bool is_interop = false, MPI_Comm* comm = nullptr
  );
  static void finalize(RuntimePtrType in_rt = nullptr);
  static void scheduleThenFinalize(
    RuntimePtrType in_rt = nullptr, WorkerCountType const workers = no_workers
  );
  static void setCurrentRuntimeTLS(RuntimeUnsafePtrType in_rt = nullptr);
  static void abort(std::string const str = "", ErrorCodeType const code = 0);
  static void output(
    std::string const str = "", ErrorCodeType const code = 1,
    bool error = false, bool decorate = true, bool formatted = false
  );

  static HandlerType registerHandler(ActiveClosureFnType fn);
};

using CollectiveOps = CollectiveAnyOps<collective_default_inst>;

// Export the default CollectiveOps::{abort,output} to the vt namespace
void abort(std::string const str = "", ErrorCodeType const code = 1);
void output(
  std::string const str = "", ErrorCodeType const code = 1, bool error = false,
  bool decorate = true
);

} //end namespace vt

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_OPS_H*/
