/*
//@HEADER
// *****************************************************************************
//
//                            pipe_manager_tl.impl.h
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

#if !defined INCLUDED_PIPE_PIPE_MANAGER_TL_IMPL_H
#define INCLUDED_PIPE_PIPE_MANAGER_TL_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager_tl.h"
#include "vt/pipe/pipe_manager_base.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/pipe/callback/handler_send/callback_send.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send_tl.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast_tl.h"
#include "vt/pipe/callback/objgroup_send/callback_objgroup_send.h"
#include "vt/pipe/callback/objgroup_bcast/callback_objgroup_bcast.h"
#include "vt/activefn/activefn.h"
#include "vt/context/context.h"
#include "vt/utils/static_checks/functor.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"
#include "vt/vrt/collection/dispatch/registry.h"
#include "vt/objgroup/proxy/proxy_bits.h"

#include <memory>

namespace vt { namespace pipe {

template <typename unused_>
PipeManagerTL::CallbackType PipeManagerTL::makeCallback() {
  bool const persist = true;
  bool const send_back = false;
  bool const dispatch = true;
  auto const& pipe_id = makePipeID(persist,send_back);
  newPipeState(pipe_id,persist,dispatch,-1,-1,0);
  auto cb = CallbackType(callback::cbunion::RawAnonTag,pipe_id);
  return cb;
}

template <typename T>
PipeManagerTL::CallbackMsgType<T> PipeManagerTL::makeCallbackTyped() {
  return makeCallback();
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
void PipeManagerTL::addListener(CallbackT const& cb, NodeType const& node) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackSend<MsgT>>(han,node)
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
void PipeManagerTL::addListenerBcast(CallbackT const& cb) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackBcast<MsgT>>(han)
  );
}

template <typename FunctorT, typename T, typename CallbackT>
void PipeManagerTL::addListenerFunctor(
  CallbackT const& cb, NodeType const& node
) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& han = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackSend<MsgT>>(han,node)
  );
}

template <typename FunctorT, typename CallbackT>
void PipeManagerTL::addListenerFunctorVoid(
  CallbackT const& cb, NodeType const& node
) {
  auto const& han = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  addListenerAny<signal::SigVoidType>(
    cb.getPipe(), std::make_unique<callback::CallbackSend<signal::SigVoidType>>(
      han,node
    )
  );
}

template <typename FunctorT, typename T, typename CallbackT>
void PipeManagerTL::addListenerFunctorBcast(
  CallbackT const& cb
) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& han = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackBcast<MsgT>>(han)
  );
}

template <
  typename ColT, typename MsgT, PipeManagerTL::ColHanType<ColT,MsgT>* f,
  typename CallbackT
>
CallbackT
PipeManagerTL::makeCallbackSingleProxySend(typename ColT::ProxyType proxy) {
  bool const persist = true;
  bool const send_back = false;
  bool const dispatch = true;
  auto const& pipe_id = makePipeID(persist,send_back);
  newPipeState(pipe_id,persist,dispatch,-1,-1,0);
  auto cb = CallbackT(callback::cbunion::RawSendColMsgTag,pipe_id);
  auto const& handler = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>();
  addListenerAny<MsgT>(
    cb.getPipe(),
    std::make_unique<callback::CallbackProxySend<ColT, MsgT>>(handler, proxy)
  );
  return cb;
}

template <
  typename ColT, typename MsgT, PipeManagerTL::ColMemType<ColT,MsgT> f,
  typename CallbackT
>
CallbackT
PipeManagerTL::makeCallbackSingleProxySend(typename ColT::ProxyType proxy) {
  bool const persist = true;
  bool const send_back = false;
  bool const dispatch = true;
  auto const& pipe_id = makePipeID(persist,send_back);
  newPipeState(pipe_id,persist,dispatch,-1,-1,0);
  auto cb = CallbackT(callback::cbunion::RawSendColMsgTag,pipe_id);
  auto const& handler =
    auto_registry::makeAutoHandlerCollectionMem<ColT,MsgT,f>();
  addListenerAny<MsgT>(
    cb.getPipe(),
    std::make_unique<callback::CallbackProxySend<ColT, MsgT>>(handler, proxy)
  );
  return cb;
}

template <
  typename ObjT, typename MsgT, PipeManagerTL::ObjMemType<ObjT,MsgT> fn,
  typename CallbackT
>
CallbackT
PipeManagerTL::makeCallbackObjGrpSend(objgroup::proxy::ProxyElm<ObjT> proxy) {
  bool const persist = true;
  bool const send_back = false;
  auto const& pipe_id = makePipeID(persist,send_back);
  auto const proxy_bits = proxy.getProxy();
  auto const dest_node = proxy.getNode();
  auto const ctrl = objgroup::proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT,MsgT,fn>(ctrl);
  auto cb = CallbackT(
    callback::cbunion::RawSendObjGrpTag,pipe_id,han,proxy_bits,dest_node
  );
  return cb;
}

template <
  typename ObjT, typename MsgT, PipeManagerTL::ObjMemType<ObjT,MsgT> fn,
  typename CallbackT
>
CallbackT
PipeManagerTL::makeCallbackObjGrpBcast(objgroup::proxy::Proxy<ObjT> proxy) {
  bool const persist = true;
  bool const send_back = false;
  auto const& pipe_id = makePipeID(persist,send_back);
  auto const proxy_bits = proxy.getProxy();
  auto const ctrl = objgroup::proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT,MsgT,fn>(ctrl);
  auto cb = CallbackT(
    callback::cbunion::RawBcastObjGrpTag,pipe_id,han,proxy_bits
  );
  return cb;
}


// Single active message collection proxy bcast
template <
  typename ColT, typename MsgT, PipeManagerTL::ColHanType<ColT,MsgT>* f,
  typename CallbackT
>
CallbackT
PipeManagerTL::makeCallbackSingleProxyBcast(ColProxyType<ColT> proxy) {
  bool const persist = true;
  bool const send_back = false;
  bool const dispatch = true;
  auto const& pipe_id = makePipeID(persist,send_back);
  newPipeState(pipe_id,persist,dispatch,-1,-1,0);
  auto cb = CallbackT(callback::cbunion::RawBcastColMsgTag,pipe_id);
  auto const& handler = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>();
  addListenerAny<MsgT>(
    cb.getPipe(),
    std::make_unique<callback::CallbackProxyBcast<ColT,MsgT>>(handler,proxy)
  );
  return cb;
}

template <
  typename ColT, typename MsgT, PipeManagerTL::ColHanType<ColT,MsgT>* f,
  typename CallbackT
>
CallbackT
PipeManagerTL::makeCallbackSingleProxyBcastDirect(ColProxyType<ColT> proxy) {
  bool const persist = true;
  bool const send_back = false;
  auto const& pipe_id = makePipeID(persist,send_back);
  auto const& handler = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>();
  auto const& vrt_handler = vrt::collection::makeVrtDispatch<MsgT,ColT>();
  auto cb = CallbackT(
    callback::cbunion::RawBcastColDirTag, pipe_id, handler, vrt_handler,
    proxy.getProxy()
  );
  return cb;
}

template <
  typename ColT, typename MsgT, PipeManagerTL::ColMemType<ColT,MsgT> f,
  typename CallbackT
>
CallbackT
PipeManagerTL::makeCallbackSingleProxyBcastDirect(ColProxyType<ColT> proxy) {
  bool const persist = true;
  bool const send_back = false;
  auto const& pipe_id = makePipeID(persist,send_back);
  auto const& handler =
    auto_registry::makeAutoHandlerCollectionMem<ColT,MsgT,f>();
  auto const& vrt_handler = vrt::collection::makeVrtDispatch<MsgT,ColT>();
  auto cb = CallbackT(
    callback::cbunion::RawBcastColDirTag, pipe_id, handler, vrt_handler,
    proxy.getProxy()
  );
  return cb;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackSingleSend(NodeType const& send_to_node) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>();
  auto cb = CallbackT(
    callback::cbunion::RawSendMsgTag,new_pipe_id,handler,send_to_node
  );
  return cb;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
CallbackT
 PipeManagerTL::makeCallbackSingleSendT(NodeType const& send_to_node) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>();
  auto cb = CallbackT(
    callback::cbunion::RawSendMsgTag,new_pipe_id,handler,send_to_node
  );
  return cb;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackSingleBcast() {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>();
  auto cb = CallbackT(
    callback::cbunion::RawBcastMsgTag,new_pipe_id,handler
  );
  return cb;
}

template <typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackSingleAnonVoid(LifetimeEnum life, FuncVoidType fn) {
  PipeType new_pipe_id = no_pipe;
  if (life == LifetimeEnum::Once) {
    new_pipe_id = makeCallbackFuncVoid(false,fn,true,1,1);
  } else {
    new_pipe_id = makeCallbackFuncVoid(true,fn,true);
  }
  auto cb = CallbackT(callback::cbunion::RawAnonTag,new_pipe_id);

  vt_debug_print(
    pipe, node,
    "makeCallbackSingleAnonVoid: persist={}, pipe={:x}\n",
    true, new_pipe_id
  );

  return cb;
}

template <typename C, typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackSingleAnon(
  LifetimeEnum life, C* ctx, FuncCtxType<C> fn
) {
  auto fn_closure = [ctx,fn] { fn(ctx); };

  vt_debug_print(
    pipe, node,
    "makeCallbackSingleAnon: created closure\n"
  );

  return makeCallbackSingleAnonVoid(life,fn_closure);
}

template <typename MsgT, typename C, typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackSingleAnon(
  LifetimeEnum life, C* ctx, FuncMsgCtxType<MsgT, C> fn
) {
  auto fn_closure = [ctx,fn](MsgT* msg) { fn(msg, ctx); };

  vt_debug_print(
    pipe, node,
    "makeCallbackSingleAnon: created closure\n"
  );

  return makeCallbackSingleAnon<MsgT,CallbackT>(life,fn_closure);
}

template <typename MsgT, typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackSingleAnon(LifetimeEnum life, FuncMsgType<MsgT> fn) {
  PipeType new_pipe_id = no_pipe;
  if (life == LifetimeEnum::Once) {
    new_pipe_id = makeCallbackFunc<MsgT>(false,fn,true,1,1);
  } else {
    new_pipe_id = makeCallbackFunc<MsgT>(true,fn,true);
  }

  auto cb = CallbackT(callback::cbunion::RawAnonTag,new_pipe_id);

  vt_debug_print(
    pipe, node,
    "makeCallbackSingleAnon: persist={}, pipe={:x}\n",
    true, new_pipe_id
  );

  return cb;
}

template <typename FunctorT, typename T, typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackFunctorSend(NodeType const& send_to_node) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler =
    auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  auto cb = CallbackT(
    callback::cbunion::RawSendMsgTag,new_pipe_id,handler,send_to_node
  );
  return cb;
}

template <typename FunctorT, typename T, typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackFunctorBcast() {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler =
    auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  auto cb = CallbackT(
    callback::cbunion::RawBcastMsgTag,new_pipe_id,handler
  );
  return cb;
}

template <typename FunctorT, typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackFunctorSendVoid(NodeType const& send_to_node) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  auto cb = CallbackT(
    callback::cbunion::RawSendMsgTag,new_pipe_id,handler,send_to_node
  );
  return cb;
}

template <typename FunctorT, typename CallbackT>
CallbackT
PipeManagerTL::makeCallbackFunctorBcastVoid() {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  auto cb = CallbackT(
    callback::cbunion::RawBcastMsgTag,new_pipe_id,handler
  );
  return cb;
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TL_IMPL_H*/
