/*
//@HEADER
// *****************************************************************************
//
//                              collective_ops.cc
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

#include "vt/collective/collective_ops.h"
#include "vt/runtime/runtime.h"
#include "vt/scheduler/scheduler.h"
#include "vt/runtime/runtime_inst.h"
#include "vt/utils/tls/tls.h"

#include <memory>
#include <cstdlib>
#include <mpi.h>

namespace vt {

template <runtime::RuntimeInstType instance>
RuntimePtrType CollectiveAnyOps<instance>::initialize(
  int& argc, char**& argv, WorkerCountType const num_workers,
  bool is_interop, MPI_Comm* comm, arguments::AppConfig const* appConfig
) {
  using vt::runtime::RuntimeInst;
  using vt::runtime::Runtime;
  using vt::runtime::eRuntimeInstance;

  MPI_Comm resolved_comm = comm not_eq nullptr ? *comm : MPI_COMM_WORLD;

#pragma sst global rt
  RuntimeInst<instance>::rt = std::make_unique<Runtime>(
    argc, argv, num_workers, is_interop, resolved_comm,
    eRuntimeInstance::DefaultInstance, appConfig
  );

#pragma sst global rt
  auto rt_ptr = RuntimeInst<instance>::rt.get();
  if (instance == runtime::RuntimeInstType::DefaultInstance) {
    // Set global variable for default instance for backward compatibility
    ::vt::rt = rt_ptr;
    curRT = rt_ptr;
  }
#pragma sst global rt
  RuntimeInst<instance>::rt->initialize();

  return runtime::makeRuntimePtr(rt_ptr);
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::setCurrentRuntimeTLS(RuntimeUnsafePtrType in) {
  bool const has_rt = in != nullptr;
  auto rt_use = has_rt ? in : ::vt::rt;
  curRT = rt_use;
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::scheduleThenFinalize(
  RuntimePtrType in_rt, WorkerCountType const workers
) {
  bool const has_rt = in_rt != nullptr;
  auto rt_use = has_rt ? in_rt.unsafe() : curRT;

  auto sched_fn = [=]{
    theSched()->runSchedulerWhile([rt_use]{ return not rt_use->isTerminated(); });
  };

  if (workers == no_workers) {
    sched_fn();
  } else {
    #if vt_threading_enabled
    theWorkerGrp()->spawnWorkersBlock(sched_fn);
    #else
    sched_fn();
    #endif
  }

  CollectiveAnyOps<instance>::finalize(has_rt ? std::move(in_rt) : nullptr);
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::finalize(RuntimePtrType in_rt) {
  using vt::runtime::RuntimeInst;
  using vt::runtime::Runtime;
  using vt::runtime::eRuntimeInstance;

#pragma sst global rt
  RuntimeInst<instance>::rt = nullptr;

  if (instance == runtime::RuntimeInstType::DefaultInstance) {
    // Set global variable for default instance for backward compatibility
    ::vt::rt = nullptr;
    curRT = nullptr;
  }

  if (in_rt) {
    in_rt = nullptr;
  }
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::abort(
  std::string const str, ErrorCodeType const code
) {
  auto tls_rt = curRT;
  auto myrt = tls_rt ? tls_rt : ::vt::rt;
  if (myrt) {
#if vt_check_enabled(trace_enabled)
    //--- Try to flush most of the traces before aborting
    myrt->theTrace->cleanupTracesFile();
#endif
    myrt->abort(str, code);
  } else if (vt::debug::preConfig()->vt_throw_on_abort) {
    // Special case when preConfig has 'vt_throw_on_abort' option set
    // This is meant to be used by nompi unit tests
    throw std::runtime_error(str);
  } else {
    std::_Exit(code);
  }
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::output(
  std::string const str, ErrorCodeType const code, bool error, bool decorate,
  bool formatted, bool abort_out
) {
  auto tls_rt = curRT;
  auto myrt = tls_rt ? tls_rt : ::vt::rt;
  if (myrt) {
    myrt->output(str,code,error,decorate,formatted);
  } else {
    ::fmt::print(str.c_str());
  }
  if (error and abort_out) {
    vt::abort("Assertion Failed", 129);
  }
}

template <runtime::RuntimeInstType instance>
HandlerType CollectiveAnyOps<instance>::registerHandler(ActiveClosureFnType fn) {
  return theRegistry()->registerActiveHandler(fn);
}

template struct CollectiveAnyOps<collective_default_inst>;

} /* end namespace vt */
