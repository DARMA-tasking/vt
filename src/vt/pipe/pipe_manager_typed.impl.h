/*
//@HEADER
// *****************************************************************************
//
//                          pipe_manager_typed.impl.h
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

#if !defined INCLUDED_PIPE_PIPE_MANAGER_TYPED_IMPL_H
#define INCLUDED_PIPE_PIPE_MANAGER_TYPED_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/interface/remote_container.h"
#include "vt/pipe/interface/send_container.h"
#include "vt/pipe/interface/callback_direct.h"
#include "vt/pipe/pipe_manager_construct.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/context/context.h"
#include "vt/registry/auto/auto_registry.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <array>

namespace vt { namespace pipe {

template <typename MsgT>
PipeManagerTyped::CallbackAnonType<MsgT>
PipeManagerTyped::makeCallbackSingleAnonTyped(
  bool const persist, FuncMsgType<MsgT> fn
) {
  auto const& new_pipe_id = makeCallbackFunc<MsgT>(persist,fn);
  auto container = CallbackAnonType<MsgT>{new_pipe_id};

  vt_debug_print(
    pipe, node,
    "makeCallbackSingleAnonTyped: persist={}, pipe={:x}\n",
    persist, new_pipe_id
  );

  return container;
}

template <typename unused_>
PipeManagerTyped::CallbackAnonVoidType
PipeManagerTyped::makeCallbackSingleAnonVoidTyped(
  bool const persist, FuncVoidType fn
) {
  auto const& new_pipe_id = makeCallbackFuncVoid(persist,fn);
  auto container = CallbackAnonVoidType{new_pipe_id};

  vt_debug_print(
    pipe, node,
    "makeCallbackSingleAnonVoidTyped: persist={}, pipe={:x}\n",
    persist, new_pipe_id
  );

  return container;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
PipeManagerTyped::CallbackSendType<MsgT>
PipeManagerTyped::makeCallbackSingleSendTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  auto container = CallbackSendType<MsgT>(
    new_pipe_id,handler,send_to_node
  );
  return container;
}

template <typename FunctorT, typename MsgT>
PipeManagerTyped::CallbackSendType<MsgT>
PipeManagerTyped::makeCallbackSingleSendFunctorTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
  auto const& handler =
    auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  auto container = CallbackSendType<MsgT>(new_pipe_id,handler,send_to_node);
  return container;
}

template <typename FunctorT>
PipeManagerTyped::CallbackSendVoidType
PipeManagerTyped::makeCallbackSingleSendFunctorVoidTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
  auto const& handler = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  auto container = CallbackSendVoidType(new_pipe_id,handler,send_to_node);
  return container;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
PipeManagerTyped::CallbackBcastType<MsgT>
PipeManagerTyped::makeCallbackSingleBcastTyped(bool const inc) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  auto container = CallbackBcastType<MsgT>(
    new_pipe_id,handler,inc
  );
  return container;
}

template <typename FunctorT, typename MsgT>
PipeManagerTyped::CallbackBcastType<MsgT>
PipeManagerTyped::makeCallbackSingleBcastFunctorTyped(bool const inc) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler =
    auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  auto container = CallbackBcastType<MsgT>(
    new_pipe_id,handler,inc
  );
  return container;
}

template <typename FunctorT>
PipeManagerTyped::CallbackBcastVoidType
PipeManagerTyped::makeCallbackSingleBcastFunctorVoidTyped(bool const inc) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  auto container = CallbackBcastVoidType(new_pipe_id,handler,inc);
  return container;
}

template <
  typename ColT,
  typename MsgT,
  vrt::collection::ActiveColTypedFnType<MsgT,ColT> *f
>
PipeManagerTyped::CallbackProxyBcastType<ColT,MsgT>
PipeManagerTyped::makeCallbackSingleProxyBcastTyped(
  CollectionProxy<ColT,typename ColT::IndexType> proxy
) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& han = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(
    nullptr
  );
  auto container = CallbackProxyBcastType<ColT,MsgT>(new_pipe_id,han,proxy);
  return container;
}

template <
  typename ColT,
  typename MsgT,
  vrt::collection::ActiveColTypedFnType<MsgT,ColT> *f
>
PipeManagerTyped::CallbackProxySendType<ColT,MsgT>
PipeManagerTyped::makeCallbackSingleProxySendTyped(
  typename ColT::ProxyType proxy
) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& han = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(
    nullptr
  );
  auto container = CallbackProxySendType<ColT,MsgT>(new_pipe_id,han,proxy);
  return container;
}

template <typename MsgT, ActiveTypedFnType<MsgT>*... f>
interface::CallbackDirectSendMulti<
  MsgT,
  typename RepeatNImpl<sizeof...(f),callback::CallbackSend<MsgT>>::ResultType
>
PipeManagerTyped::makeCallbackMultiSendTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  using CBSendT = callback::CallbackSend<MsgT>;
  using ConsT = std::tuple<NodeType>;
  using TupleConsT = typename RepeatNImpl<sizeof...(f),ConsT>::ResultType;
  using ConstructMeta = ConstructCallbacks<CBSendT,TupleConsT,MsgT,f...>;
  using TupleCBType = typename ConstructMeta::ResultType;

  auto const& new_pipe_id = makePipeID(is_persist,false);
  std::array<NodeType,sizeof...(f)> send_node_array;
  send_node_array.fill(send_to_node);
  auto const cons = TupleConsT{send_node_array};
  auto const tuple = ConstructMeta::make(cons);
  auto rcm = interface::CallbackDirectSendMulti<MsgT,TupleCBType>(
    interface::CallbackDirectSendMultiTag,new_pipe_id,tuple
  );
  return rcm;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
auto PipeManagerTyped::pushTargetBcast(CallbackT in, bool const& inc) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return std::tuple_cat(
    std::make_tuple(callback::CallbackBcast<MsgT>(han,inc)), in
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
auto PipeManagerTyped::pushTargetBcast(bool const& inc) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return std::make_tuple(callback::CallbackBcast<MsgT>(han,inc));
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
auto PipeManagerTyped::pushTarget(CallbackT in, NodeType const& send_to_node) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return std::tuple_cat(
    std::make_tuple(callback::CallbackSend<MsgT>(han,send_to_node)), in
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
auto PipeManagerTyped::pushTarget(NodeType const& send_to_node) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return std::make_tuple(callback::CallbackSend<MsgT>(han,send_to_node));
}

template <typename CallbackT>
auto PipeManagerTyped::buildMultiCB(CallbackT in) {
  using MsgT = typename std::tuple_element<0,CallbackT>::type::SignalDataType;
  auto const& new_pipe_id = makePipeID(true,false);
  return interface::CallbackDirectSendMulti<MsgT,CallbackT>(
    interface::CallbackDirectSendMultiTag,new_pipe_id,in
  );
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TYPED_IMPL_H*/
