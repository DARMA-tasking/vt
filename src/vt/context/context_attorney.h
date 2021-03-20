/*
//@HEADER
// *****************************************************************************
//
//                              context_attorney.h
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

#if !defined INCLUDED_CONTEXT_CONTEXT_ATTORNEY_H
#define INCLUDED_CONTEXT_CONTEXT_ATTORNEY_H

#include "vt/config.h"
#include "vt/context/context_attorney_fwd.h"
#include "vt/worker/worker_headers.h"
#include "vt/runtime/runtime_headers.h"
#include "vt/runnable/runnable.fwd.h"
#include "vt/context/context_runnable/set_context.fwd.h"

namespace vt {  namespace ctx {

/** \file */

/**
 * \struct ContextAttorney context_attorney.h vt/context/context_attorney.h
 *
 * \brief Used by VT internals for write access to the Context
 *
 * Attorney pattern to Context for setting number of workers and current worker
 * by the runtime and other components
 */
struct ContextAttorney {
  #if vt_threading_enabled
  /// Allow the worker to modify the contextual worker
  friend worker::WorkerGroupType;
  /// Allow the worker group to modify the contextual worker
  friend worker::WorkerType;
  #endif
  /// Allow the runtime to set the number of workers
  friend runtime::Runtime;

  /// Allow \c ctx::SetContext to modify the running task
  friend ctx::SetContext;

private:
  /// Allow internal runtime to set the worker
  static void setWorker(WorkerIDType const worker);
  /// Allow internal runtime to set the number of workers
  static void setNumWorkers(WorkerCountType const worker_count);
  /// Allow setting the current running task
  static void setTask(runnable::RunnableNew* in_task);
};

}} /* end namespace vt::ctx */

#endif /*INCLUDED_CONTEXT_CONTEXT_ATTORNEY_H*/
