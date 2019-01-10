/*
//@HEADER
// ************************************************************************
//
//                          general.impl.h
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

#if !defined INCLUDED_RUNNABLE_GENERAL_IMPL_H
#define INCLUDED_RUNNABLE_GENERAL_IMPL_H

#include "vt/config.h"
#include "vt/runnable/general.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/trace/trace_common.h"
#include "vt/messaging/envelope.h"
#include "vt/handler/handler.h"

#include <cassert>

namespace vt { namespace runnable {

template <typename MsgT>
/*static*/ void Runnable<MsgT>::run(
  HandlerType handler, ActiveFnPtrType func, MsgT* msg, NodeType from_node,
  TagType in_tag
) {
  using HandlerManagerType = HandlerManager;

  #if backend_check_enabled(trace_enabled)
    trace::TraceEntryIDType trace_id = auto_registry::theTraceID(
      handler, auto_registry::RegistryTypeEnum::RegGeneral
    );
    trace::TraceEventIDType trace_event = trace::no_trace_event;
    if (msg) {
      trace_event = envelopeGetTraceEvent(msg->env);
    }
  #endif

  #if backend_check_enabled(trace_enabled)
    theTrace()->beginProcessing(trace_id, sizeof(MsgT), trace_event, from_node);
  #endif

  bool is_functor = false;
  bool is_auto = false;
  bool bare_handler = false;
  auto_registry::NumArgsType num_args = 1;

  if (func == nullptr) {
    is_auto = HandlerManagerType::isHandlerAuto(handler);
    is_functor = HandlerManagerType::isHandlerFunctor(handler);

    if (is_auto && is_functor) {
      func = auto_registry::getAutoHandlerFunctor(handler);
      num_args = auto_registry::getAutoHandlerFunctorArgs(handler);
    } else if (is_auto) {
      func = auto_registry::getAutoHandler(handler);
    } else {
      auto typed_func = theRegistry()->getHandler(handler, in_tag);
      typed_func(msg);
      bare_handler = true;
    }
  }

  if (num_args == 0) {
    auto no_arg_fn = reinterpret_cast<FnParamType<>>(func);
    no_arg_fn();
  } else if (!bare_handler) {
    func(msg);
  }

  #if backend_check_enabled(trace_enabled)
    theTrace()->endProcessing(trace_id, sizeof(MsgT), trace_event, from_node);
  #endif
}

/*static*/ inline void RunnableVoid::run(
  HandlerType handler, NodeType from_node
) {

  using HandlerManagerType = HandlerManager;

  bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);

  ActiveFnPtrType func = nullptr;

  if (is_auto && is_functor) {
    func = auto_registry::getAutoHandlerFunctor(handler);
  } else if (is_auto) {
    func = auto_registry::getAutoHandler(handler);
  } else {
    vtAssert(0, "Must be auto handler");
  }

  auto void_fn = reinterpret_cast<FnParamType<>>(func);

  void_fn();
}

}} /* end namespace vt::runnable */

#endif /*INCLUDED_RUNNABLE_GENERAL_IMPL_H*/
