/*
//@HEADER
// *****************************************************************************
//
//                             pipe_manager_typed.h
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

#if !defined INCLUDED_PIPE_PIPE_MANAGER_TYPED_H
#define INCLUDED_PIPE_PIPE_MANAGER_TYPED_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager_base.h"
#include "vt/pipe/pipe_manager_typed.fwd.h"
#include "vt/pipe/interface/send_container.h"
#include "vt/pipe/interface/callback_direct.h"
#include "vt/pipe/interface/callback_direct_multi.h"
#include "vt/pipe/interface/callback_types.h"
#include "vt/pipe/callback/handler_send/callback_send_han.h"
#include "vt/pipe/pipe_manager_construct.h"
#include "vt/utils/static_checks/functor.h"
#include "vt/vrt/collection/active/active_funcs.h"

#include <type_traits>
#include <tuple>
#include <functional>

namespace vt { namespace pipe {

struct PipeManagerTyped : virtual PipeManagerBase {
  template <typename T>
  using CBTypes                = interface::CallbackTypes<T>;
  template <typename T>
  using CallbackSendType       = typename CBTypes<T>::CallbackDirectSend;
  template <typename T>
  using CallbackAnonType       = typename CBTypes<T>::CallbackAnon;
  template <typename T>
  using CallbackBcastType      = typename CBTypes<T>::CallbackDirectBcast;
  using CallbackSendVoidType   = typename CBTypes<void>::CallbackDirectVoidSend;
  using CallbackBcastVoidType  = typename CBTypes<void>::CallbackDirectVoidBcast;
  using CallbackAnonVoidType   = typename CBTypes<void>::CallbackAnonVoid;

  template <typename C, typename T>
  using CBVrtTypes             = interface::CallbackVrtTypes<C,T>;
  template <typename C, typename T>
  using CallbackProxyBcastType = typename CBVrtTypes<C,T>::CallbackProxyBcast;
  template <typename C, typename T>
  using CallbackProxySendType  = typename CBVrtTypes<C,T>::CallbackProxySend;

  /*
   *  Builders for non-send-back type of pipe callback: they are invoked
   *  directly from the sender; thus this node is not involved in the process
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>*... f>
  interface::CallbackDirectSendMulti<
    MsgT,
    typename RepeatNImpl<sizeof...(f),callback::CallbackSend<MsgT>>::ResultType
  >
  makeCallbackMultiSendTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  auto pushTargetBcast(bool const& inc);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
  auto pushTargetBcast(CallbackT in, bool const& inc);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  auto pushTarget(NodeType const& send_to_node);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
  auto pushTarget(CallbackT in, NodeType const& send_to_node);

  template <typename CallbackT>
  auto buildMultiCB(CallbackT in);

  /*
   *  Make a fully typed single-endpoint listener callback that directly sends
   *  to the endpoint
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackSendType<MsgT> makeCallbackSingleSendTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  template <
    typename FunctorT,
    typename MsgT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  CallbackSendType<MsgT> makeCallbackSingleSendFunctorTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  template <typename FunctorT>
  CallbackSendVoidType makeCallbackSingleSendFunctorVoidTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  /*
   *  Make a fully typed single-endpoint anon function callback
   */
  template <typename MsgT>
  CallbackAnonType<MsgT> makeCallbackSingleAnonTyped(
    bool const persist, FuncMsgType<MsgT> fn
  );

  template <typename=void>
  CallbackAnonVoidType makeCallbackSingleAnonVoidTyped(
    bool const persist, FuncVoidType fn
  );

  /*
   *  Make a fully typed single-endpoint broadcast directly routed callback
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackBcastType<MsgT> makeCallbackSingleBcastTyped(bool const inc);

  template <
    typename FunctorT,
    typename MsgT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  CallbackBcastType<MsgT> makeCallbackSingleBcastFunctorTyped(bool const inc);

  template <typename FunctorT>
  CallbackBcastVoidType makeCallbackSingleBcastFunctorVoidTyped(bool const inc);

  /*
   *  Make a fully typed vrt proxy broadcast directly routed callback
   */
  template <
    typename ColT,
    typename MsgT,
    vrt::collection::ActiveColTypedFnType<MsgT,ColT> *f
  >
  CallbackProxyBcastType<ColT,MsgT> makeCallbackSingleProxyBcastTyped(
    CollectionProxy<ColT,typename ColT::IndexType> proxy
  );

  /*
   *  Make a fully typed vrt proxy indexed send directly routed callback
   */
  template <
    typename ColT,
    typename MsgT,
    vrt::collection::ActiveColTypedFnType<MsgT,ColT> *f
  >
  CallbackProxySendType<ColT,MsgT> makeCallbackSingleProxySendTyped(
    typename ColT::ProxyType proxy
  );
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TYPED_H*/
