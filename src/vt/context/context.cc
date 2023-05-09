/*
//@HEADER
// *****************************************************************************
//
//                                  context.cc
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

#include "vt/context/context.h"
#include "vt/context/runnable_context/set_context.h"

#if vt_check_enabled(trace_only)
namespace vt { namespace runnable {
struct RunnableNew {};
}} /* end namespace vt::runnable */
#else
# include "vt/runtime/runtime.h"
# include "vt/runnable/runnable.h"
#endif

#if vt_check_enabled(trace_enabled)
#include "vt/trace/trace_common.h"
#if !vt_check_enabled(trace_only)
#include "vt/context/runnable_context/trace.h"
#endif
#endif

#include <string>
#include <cstring>

#include <mpi.h>

// This cannot use the normal debug_print macros because they rely on context
// being live to print contextual information
#define DEBUG_VT_CONTEXT 0

namespace vt { namespace ctx {

Context::Context(bool const is_interop, MPI_Comm comm) {
  #if DEBUG_VT_CONTEXT
    fmt::print(
      "Context::Context is_interop={}, comm={}\n", print_bool(is_interop), comm
    );
  #endif

  // Always duplicate, which may be MPI_COMM_WORLD.
  MPI_Comm_dup(comm, &comm);

  int numNodesLocal = uninitialized_destination;
  int thisNodeLocal = uninitialized_destination;

  MPI_Comm_size(comm, &numNodesLocal);
  MPI_Comm_rank(comm, &thisNodeLocal);

  communicator_ = comm;
  numNodes_ = static_cast<NodeType>(numNodesLocal);
  thisNode_ = static_cast<NodeType>(thisNodeLocal);
}

Context::~Context() {
  MPI_Comm_free(&communicator_);
}

void Context::setTask(runnable::RunnableNew* in_task) {
  cur_task_ = in_task;
}

NodeType Context::getFromNodeCurrentTask() const {
#if !vt_check_enabled(trace_only)
  if (getTask() != nullptr) {
    auto from = getTask()->get<ctx::SetContext>();
    if (from != nullptr) {
      return from->get();
    }
  }
#endif
  return getNode();
}

#if vt_check_enabled(trace_enabled)
trace::TraceEventIDType Context::getTraceEventCurrentTask() const {
#if !vt_check_enabled(trace_only)
  if (getTask() != nullptr) {
    auto t = getTask()->get<ctx::Trace>();
    if (t != nullptr) {
      return t->getEvent();
    }
  }
#endif
  return trace::no_trace_event;
}
#endif /* vt_check_enabled(trace_enabled) */

}}  // end namespace vt::ctx

namespace vt { namespace debug {

NodeType preNode() {
  #if !vt_check_enabled(trace_only)
  return ::vt::curRT != nullptr and ::vt::curRT->isLive() ?
    theContext()->getNode() :
    -1;
  #else
    return theContext() ? theContext()->getNode() : -1;
  #endif
}
NodeType preNodes() {
  #if !vt_check_enabled(trace_only)
  return ::vt::curRT != nullptr and ::vt::curRT->isLive() ?
    theContext()->getNumNodes() :
    -1;
  #else
    return theContext() ? theContext()->getNode() : -1;
  #endif
}

}} /* end namespace vt::debug */


#undef DEBUG_VT_CONTEXT
