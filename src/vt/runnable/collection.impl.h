/*
//@HEADER
// ************************************************************************
//
//                          collection.impl.h
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

#if !defined INCLUDED_RUNNABLE_COLLECTION_IMPL_H
#define INCLUDED_RUNNABLE_COLLECTION_IMPL_H

#include "vt/config.h"
#include "vt/runnable/collection.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"
#include "vt/trace/trace_common.h"
#include "vt/messaging/envelope.h"
#include "vt/messaging/active.h"

namespace vt { namespace runnable {

template <typename MsgT, typename ElementT>
/*static*/ void RunnableCollection::run(
  HandlerType handler, MsgT* msg, std::size_t msg_size, ElementT* elm,
  NodeType from, bool member,
  uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4
) {
  bool const is_fetch = false;

# if backend_check_enabled(trace_enabled)
    trace::TraceEventIDType trace_event = envelopeGetTraceEvent(msg->env);
    RunnableCollection::prelude(
      trace_event, msg_size, handler, from_node, member, is_fetch,
      idx1, idx2, idx3, idx4
    );
# endif

  // Dispatching a regular message handler; test if its a pointer-to-member to
  // C-style handler on the object
  if (member) {
    auto const func = auto_registry::getAutoHandlerCollectionMem(handler);
    (elm->*func)(msg);
  } else {
    auto const func = auto_registry::getAutoHandlerCollection(handler);
    func(msg, elm);
  }

# if backend_check_enabled(trace_enabled)
    RunnableCollection::epilog(
      trace_event, msg_size, from_node, member, is_fetch, idx1, idx2, idx3, idx4
    );
# endif
}

template <typename FetchT, typename ElementT>
/*static*/ void RunnableCollection::runFetch(
  HandlerType handler, FetchT* msg, std::size_t fetch_size, ElementT* elm,
  NodeType from_node, bool member,
  uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4
) {
  bool const is_fetch = true;

# if backend_check_enabled(trace_enabled)
    RunnableCollection::prelude(
      trace_event, fetch_size, handler, from_node, member, is_fetch,
      idx1, idx2, idx3, idx4
    );
# endif

  // Dispatching a fetch handler here, always a pointer-to-member function
  // invocation
  auto const func = auto_registry::getAutoHandlerCollectionFetch(handler);
  (elm->*func)(msg);

# if backend_check_enabled(trace_enabled)
    RunnableCollection::epilog(
      trace_event, fetch_size, from_node, member, is_fetch,
      idx1, idx2, idx3, idx4
    );
# endif
}

}} /* end namespace vt::runnable */

#endif /*INCLUDED_RUNNABLE_COLLECTION_IMPL_H*/
