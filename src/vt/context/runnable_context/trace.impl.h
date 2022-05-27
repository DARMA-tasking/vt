/*
//@HEADER
// *****************************************************************************
//
//                                 trace.impl.h
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

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_IMPL_H
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_IMPL_H

#if vt_check_enabled(trace_enabled)

#include "vt/context/runnable_context/trace.h"
#include "vt/handler/handler.h"
#include "vt/serialization/sizer.h"

namespace vt { namespace ctx {

template <typename MsgT>
Trace::Trace(
  MsgT const& msg, HandlerType const in_handler, NodeType const in_from_node
) : is_collection_(false),
    event_(envelopeGetTraceEvent(msg->env)),
    msg_size_(
      vt::serialization::MsgSizer<typename MsgT::MsgType>::get(msg.get())
    ),
    is_traced_(HandlerManager::isHandlerTrace(in_handler)),
    from_node_(in_from_node),
    handler_(in_handler)
{ }

template <typename MsgT>
Trace::Trace(
  MsgT const& msg, trace::TraceEventIDType const in_trace_event,
  HandlerType const in_handler, NodeType const in_from_node,
  uint64_t in_idx1, uint64_t in_idx2, uint64_t in_idx3, uint64_t in_idx4
) : is_collection_(true),
    event_(in_trace_event),
    msg_size_(
      vt::serialization::MsgSizer<typename MsgT::MsgType>::get(msg.get())
    ),
    is_traced_(HandlerManager::isHandlerTrace(in_handler)),
    from_node_(in_from_node),
    handler_(in_handler),
    idx1_(in_idx1),
    idx2_(in_idx2),
    idx3_(in_idx3),
    idx4_(in_idx4)
{ }

}} /* end namespace vt::ctx */

#endif /*vt_check_enabled(trace_enabled)*/
#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_IMPL_H*/
