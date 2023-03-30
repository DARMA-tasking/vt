/*
//@HEADER
// *****************************************************************************
//
//                               collective_ops.h
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

#if !defined INCLUDED_VT_COLLECTIVE_COLLECTIVE_OPS_H
#define INCLUDED_VT_COLLECTIVE_COLLECTIVE_OPS_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/runtime/runtime_headers.h"

#include <string>

#include <mpi.h>

namespace vt {

static constexpr runtime::RuntimeInstType const collective_default_inst =
  runtime::RuntimeInstType::DefaultInstance;

template <runtime::RuntimeInstType instance = collective_default_inst>
struct CollectiveAnyOps {
  // The general methods that interact with the managed runtime holder
  static RuntimePtrType initialize(
    int& argc, char**& argv, bool is_interop = false, MPI_Comm* comm = nullptr,
    arguments::AppConfig const* appConfig = nullptr
  );
  [[deprecated]] static RuntimePtrType initialize(
    int& argc, char**& argv, PhysicalResourceType const /* num_workers */,
    bool is_interop = false, MPI_Comm* comm = nullptr,
    arguments::AppConfig const* appConfig = nullptr
    )
  {
    return initialize(argc, argv, is_interop, comm, appConfig);
  }
  static void finalize(RuntimePtrType in_rt = nullptr);
  static void scheduleThenFinalize(RuntimePtrType in_rt = nullptr);
  static void setCurrentRuntimeTLS(RuntimeUnsafePtrType in_rt = nullptr);
  static void abort(std::string const str = "", ErrorCodeType const code = 0);
  static void output(
    std::string const str = "", ErrorCodeType const code = 1,
    bool error = false, bool decorate = true, bool formatted = false,
    bool abort_out = false
  );
};

using CollectiveOps = CollectiveAnyOps<collective_default_inst>;

} //end namespace vt

#endif /*INCLUDED_VT_COLLECTIVE_COLLECTIVE_OPS_H*/
