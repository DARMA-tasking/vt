/*
//@HEADER
// *****************************************************************************
//
//                              collection.impl.h
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
#include "vt/serialization/sizer.h"

namespace vt { namespace runnable {

template <typename MsgT, typename ElementT>
/*static*/ void RunnableCollection<MsgT,ElementT>::run(
  HandlerType handler, MsgT* msg, ElementT* elm, NodeType from,
  bool member, uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4,
  trace::TraceEventIDType in_trace_event
) {
#if backend_check_enabled(trace_enabled)
  trace::TraceProcessingTag processing_tag;
  {
    auto reg_enum = member ?
      auto_registry::RegistryTypeEnum::RegVrtCollectionMember :
      auto_registry::RegistryTypeEnum::RegVrtCollection;
    trace::TraceEntryIDType trace_id = auto_registry::handlerTraceID(
      handler, reg_enum
    );
    trace::TraceEventIDType trace_event = in_trace_event;
    auto const ctx_node = theMsg()->getFromNodeCurrentHandler();
    auto const from_node = from != uninitialized_destination ? from : ctx_node;

    auto const msg_size = vt::serialization::MsgSizer<MsgT>::get(msg);

    processing_tag =
      theTrace()->beginProcessing(
        trace_id, msg_size, trace_event, from_node,
        trace::Trace::getCurrentTime(), idx1, idx2, idx3, idx4
      );
  }
#endif

  if (member) {
    auto const func = auto_registry::getAutoHandlerCollectionMem(handler);
    (elm->*func)(msg);
  } else {
    auto const func = auto_registry::getAutoHandlerCollection(handler);
    func(msg, elm);
  };

#if backend_check_enabled(trace_enabled)
  theTrace()->endProcessing(processing_tag);
#endif
}

}} /* end namespace vt::runnable */

#endif /*INCLUDED_RUNNABLE_COLLECTION_IMPL_H*/
