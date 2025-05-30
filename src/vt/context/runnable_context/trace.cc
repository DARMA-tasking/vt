/*
//@HEADER
// *****************************************************************************
//
//                                   trace.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_CC
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_CC

#include "vt/config.h"

#if vt_check_enabled(trace_enabled)

#include "vt/context/runnable_context/trace.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/messaging/active.h"

namespace vt { namespace ctx {

Trace::Trace(
  trace::TraceEventIDType event, HandlerType const in_handler,
  NodeType const in_from_node, std::size_t msg_size
) : is_collection_(false),
    event_(event),
    msg_size_(msg_size),
    is_traced_(HandlerManager::isHandlerTrace(in_handler)),
    from_node_(in_from_node),
    handler_(in_handler)
{ }

Trace::Trace(
  trace::TraceEventIDType event, HandlerType const in_handler,
  NodeType const in_from_node, std::size_t msg_size,
  uint64_t in_idx1, uint64_t in_idx2, uint64_t in_idx3, uint64_t in_idx4
) : is_collection_(false),
    event_(event),
    msg_size_(msg_size),
    is_traced_(HandlerManager::isHandlerTrace(in_handler)),
    from_node_(in_from_node),
    handler_(in_handler),
    idx1_(in_idx1),
    idx2_(in_idx2),
    idx3_(in_idx3),
    idx4_(in_idx4)
{ }

void Trace::start(TimeType time) {
  if (not is_traced_) {
    return;
  }

  // If our scheduler depth is zero, we need to end the between scheduler event
  if (theSched()->getSchedulerDepth() == 0 and not theTrace()->inInvokeContext()) {
    at_sched_depth_zero_ = true;
    theTrace()->setInInvokeContext(true);
    auto const end_between_sched_time = theTrace()->beginSchedulerLoop();
    time = std::max(time, end_between_sched_time);
  }

  auto const trace_id = auto_registry::handlerTraceID(handler_);

  if (is_collection_) {
    auto const cur_node = theContext()->getFromNodeCurrentTask();
    auto const from_node =
      from_node_ != uninitialized_destination ? from_node_ : cur_node;

    processing_tag_ = theTrace()->beginProcessing(
      trace_id, msg_size_, event_, from_node, time, idx1_, idx2_, idx3_, idx4_
    );
  } else {
    processing_tag_ = theTrace()->beginProcessing(
      trace_id, msg_size_, event_, from_node_, time
    );
  }
}

void Trace::finish(TimeType time) {
  if (not is_traced_) {
    return;
  }

  theTrace()->endProcessing(processing_tag_, time);

  if (at_sched_depth_zero_) {
    theTrace()->endSchedulerLoop();
    theTrace()->setInInvokeContext(false);
  }
}

void Trace::suspend(TimeType time) {
  finish(time);
}

void Trace::resume(TimeType time) {
  // @todo: connect up the last event to this new one after suspension
  start(time);
}

}} /* end namespace vt::ctx */

#endif /*vt_check_enabled(trace_enabled)*/
#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_CC*/
