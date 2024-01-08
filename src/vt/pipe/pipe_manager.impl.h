/*
//@HEADER
// *****************************************************************************
//
//                             pipe_manager.impl.h
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

#if !defined INCLUDED_VT_PIPE_PIPE_MANAGER_IMPL_H
#define INCLUDED_VT_PIPE_PIPE_MANAGER_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/pipe/pipe_manager.fwd.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/interface/remote_container.h"
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

template <typename C>
Callback<> PipeManager::makeFunc(
  LifetimeEnum life, C* ctx, FuncCtxType<C> fn
) {
  return makeCallbackSingleAnon<C>(life,ctx,fn);
}

template <typename MsgT, typename C>
Callback<MsgT> PipeManager::makeFunc(
  LifetimeEnum life, C* ctx, FuncMsgCtxType<MsgT, C> fn
) {
  return makeCallbackSingleAnon<MsgT,C>(life,ctx,fn);
}

template <typename MsgT>
Callback<MsgT> PipeManager::makeFunc(LifetimeEnum life, FuncMsgType<MsgT> fn) {
  return makeCallbackSingleAnon<MsgT>(life,fn);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
Callback<MsgT> PipeManager::makeSend(NodeType const& node) {
  return makeCallbackSingle<f, false>(node);
}

template <typename FunctorT>
auto PipeManager::makeSend(NodeType node) {
  return makeCallbackFunctor<FunctorT, false>(node);
}

template <typename FunctorT>
auto PipeManager::makeBcast() {
  return makeCallbackFunctor<FunctorT, true>();
}

template <auto f>
auto PipeManager::makeBcast() {
  return makeCallbackSingle<f, true>();
}

template <auto f, typename Target>
auto PipeManager::makeSend(Target target) {
  if constexpr (
    std::is_same_v<Target, NodeType> or
    std::is_same_v<Target, int> or
    std::is_same_v<Target, vt::Node>
  ) {
    return makeCallbackSingle<f, false>(target);
  } else {
    // target is a proxy
    return makeCallbackProxy<f, false>(target);
  }
}

template <typename ColT, typename MsgT, PipeManager::ColHanType<ColT,MsgT>* f>
Callback<MsgT> PipeManager::makeSend(typename ColT::ProxyType proxy) {
  return makeCallbackProxy<f, false>(proxy);
}

template <typename ColT, typename MsgT, PipeManager::ColMemType<ColT,MsgT> f>
Callback<MsgT> PipeManager::makeSend(typename ColT::ProxyType proxy) {
  return makeCallbackProxy<f, false>(proxy);
}

template <typename ObjT, typename MsgT, PipeManager::ObjMemType<ObjT,MsgT> f>
Callback<MsgT> PipeManager::makeSend(objgroup::proxy::ProxyElm<ObjT> proxy) {
  return makeCallbackProxy<f, false>(proxy);
}

template <auto f, typename ProxyT>
auto PipeManager::makeBcast(ProxyT proxy) {
  return makeCallbackProxy<f, true>(proxy);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
Callback<MsgT> PipeManager::makeBcast() {
  return makeCallbackSingle<f, true>();
}

template <typename ColT, typename MsgT, PipeManager::ColHanType<ColT,MsgT>* f>
Callback<MsgT> PipeManager::makeBcast(ColProxyType<ColT> proxy) {
  return makeCallbackProxy<f, true>(proxy);
}

template <typename ColT, typename MsgT, PipeManager::ColMemType<ColT,MsgT> f>
Callback<MsgT> PipeManager::makeBcast(ColProxyType<ColT> proxy) {
  return makeCallbackProxy<f, true>(proxy);
}

template <typename ObjT, typename MsgT, PipeManager::ObjMemType<ObjT,MsgT> f>
Callback<MsgT> PipeManager::makeBcast(objgroup::proxy::Proxy<ObjT> proxy) {
  return makeCallbackProxy<f, true>(proxy);
}

template <typename MsgT>
void triggerSendBack(PipeType const& pipe, MsgT* data) {
  return theCB()->triggerSendBack(pipe,data);
}

template <typename MsgT>
void triggerPipeTyped(PipeType const& pipe, MsgT* msg) {
  return theCB()->triggerPipeTyped(pipe,msg);
}

template <typename MsgT>
void triggerPipeUnknown(PipeType const& pipe, MsgT* msg) {
  return theCB()->triggerPipeUnknown(pipe,msg);
}

template <typename MsgT>
void triggerCallbackMsgHan(MsgT* msg) {
  return PipeManager::triggerCallbackMsgHan(msg);
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_VT_PIPE_PIPE_MANAGER_IMPL_H*/
