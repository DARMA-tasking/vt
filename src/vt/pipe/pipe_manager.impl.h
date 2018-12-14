/*
//@HEADER
// ************************************************************************
//
//                          pipe_manager.impl.h
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

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/interface/remote_container_msg.h"
#include "vt/pipe/interface/send_container.h"
#include "vt/pipe/interface/callback_direct.h"
#include "vt/pipe/signal/signal_holder.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/callback/anon/callback_anon_listener.h"
#include "vt/context/context.h"
#include "vt/messaging/envelope.h"
#include "vt/registry/auto/auto_registry.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <array>

namespace vt { namespace pipe {

template <typename MsgT>
void PipeManager::triggerSendBack(PipeType const& pipe, MsgT* data) {
  auto const& this_node = theContext()->getNode();
  auto const& node_back = PipeIDBuilder::getNode(pipe);
  if (node_back != this_node) {
    // Send the message back to the owner node
    vtAssertExpr(0);
  } else {
    // Directly trigger the action because the pipe meta-data is located here
    vtAssertExpr(0);
  }
}

template <typename MsgT, typename C>
Callback<MsgT> PipeManager::makeFunc(C* ctx, FuncMsgCtxType<MsgT, C> fn) {
  return makeCallbackSingleAnon<MsgT,C,Callback<MsgT>>(ctx,fn);
}

template <typename MsgT>
Callback<MsgT> PipeManager::makeFunc(FuncMsgType<MsgT> fn) {
  return makeCallbackSingleAnon<MsgT,Callback<MsgT>>(fn);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
Callback<MsgT> PipeManager::makeSend(NodeType const& node) {
  return makeCallbackSingleSend<MsgT,f>(node);
}

template <typename FunctorT, typename MsgT>
Callback<MsgT> PipeManager::makeSend(NodeType const& node) {
  return makeCallbackFunctorSend<FunctorT,MsgT>(node);
}

template <typename FunctorT, typename not_used_>
Callback<PipeManager::Void> PipeManager::makeSend(NodeType const& node) {
  return makeCallbackFunctorSendVoid<FunctorT>(node);
}

template <typename ColT, typename MsgT, PipeManager::ColHanType<ColT,MsgT>* f>
Callback<MsgT> PipeManager::makeSend(typename ColT::ProxyType proxy) {
  return makeCallbackSingleProxySend<ColT,MsgT,f>(proxy);
}

template <typename ColT, typename MsgT, PipeManager::ColMemType<ColT,MsgT> f>
Callback<MsgT> PipeManager::makeSend(typename ColT::ProxyType proxy) {
  return makeCallbackSingleProxySend<ColT,MsgT,f>(proxy);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
Callback<MsgT> PipeManager::makeBcast() {
  return makeCallbackSingleBcast<MsgT,f>(true);
}

template <typename FunctorT, typename MsgT>
Callback<MsgT> PipeManager::makeBcast() {
  return makeCallbackFunctorBcast<FunctorT,MsgT>(true);
}

template <typename FunctorT, typename not_used_>
Callback<PipeManager::Void> PipeManager::makeBcast() {
  return makeCallbackFunctorBcastVoid<FunctorT>(true);
}

template <typename ColT, typename MsgT, PipeManager::ColHanType<ColT,MsgT>* f>
Callback<MsgT> PipeManager::makeBcast(ColProxyType<ColT> proxy) {
  return makeCallbackSingleProxyBcastDirect<ColT,MsgT,f>(proxy);
}

template <typename ColT, typename MsgT, PipeManager::ColMemType<ColT,MsgT> f>
Callback<MsgT> PipeManager::makeBcast(ColProxyType<ColT> proxy) {
  return makeCallbackSingleProxyBcastDirect<ColT,MsgT,f>(proxy);
}

}} /* end namespace vt::pipe */
