/*
//@HEADER
// *****************************************************************************
//
//                              pipe_manager_tl.h
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

#if !defined INCLUDED_VT_PIPE_PIPE_MANAGER_TL_H
#define INCLUDED_VT_PIPE_PIPE_MANAGER_TL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager_base.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"
#include "vt/pipe/pipe_lifetime.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/objgroup/active_func/active_func.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/proxy/proxy_objgroup_elm.h"

namespace vt { namespace pipe {

struct PipeManagerTL : virtual PipeManagerBase {
  template <typename ColT, typename MsgT>
  using ColHanType = vrt::collection::ActiveColTypedFnType<MsgT,ColT>;

  template <typename ColT, typename MsgT>
  using ColMemType = vrt::collection::ActiveColMemberTypedFnType<MsgT,ColT>;

  template <typename ObjT, typename MsgT>
  using ObjMemType = objgroup::ActiveObjType<MsgT,ObjT>;

  template <typename ColT>
  using ColProxyType = CollectionProxy<ColT,typename ColT::IndexType>;

  using CallbackType = callback::cbunion::CallbackRawBaseSingle;

  template <typename MsgT>
  using CallbackMsgType = callback::cbunion::CallbackTyped<MsgT>;

  template <typename... Args>
  using CallbackRetType = callback::cbunion::CallbackTyped<Args...>;

  template <typename MsgT>
  using FnType = ActiveTypedFnType<MsgT>;

  template <typename MsgT>
  using DefType = CallbackMsgType<MsgT>;

  using V = signal::SigVoidType;

  /*
   *  Untyped variants of callbacks: uses union to dispatch
   */
  // Single active message function-handler
  template <auto f, bool is_bcast>
  auto makeCallbackSingle(NodeType node = uninitialized_destination);

  template <typename FunctorT, bool is_bcast>
  auto makeCallbackFunctor(NodeType node = uninitialized_destination);

  // Single active message anon func-handler
  auto makeCallbackSingleAnonVoid(LifetimeEnum life, FuncVoidType fn);

  template <typename T>
  auto makeCallbackSingleAnon(LifetimeEnum life, FuncMsgType<T> fn);

  template <typename T, typename U>
  auto makeCallbackSingleAnon(LifetimeEnum life, U* u, FuncMsgCtxType<T, U> fn);

  template <typename U>
  auto makeCallbackSingleAnon(LifetimeEnum life, U* u, FuncCtxType<U> fn);

  // Proxy callback
  template <auto f, bool is_bcast, typename ProxyT>
  auto makeCallbackProxy(ProxyT proxy);

  // Multi-staged callback
  template <typename=void>
  CallbackType makeCallback();

  template <typename T>
  CallbackMsgType<T> makeCallbackTyped();

  template <typename T, ActiveTypedFnType<T>* f, typename CbkT = DefType<T>>
  void addListener(CbkT const& cb, NodeType const& node);

  template <typename T, ActiveTypedFnType<T>* f, typename CbkT = DefType<T>>
  void addListenerBcast(CbkT const& cb);

  template <
    typename FunctorT,
    typename T = typename util::FunctorExtractor<FunctorT>::MessageType,
    typename CbkT = DefType<T>
  >
  void addListenerFunctor(CbkT const& cb, NodeType const& node);

  template <typename FunctorT, typename CbkT = DefType<V>>
  void addListenerFunctorVoid(CbkT const& cb, NodeType const& node);

  template <
    typename FunctorT,
    typename T = typename util::FunctorExtractor<FunctorT>::MessageType,
    typename CbkT = DefType<T>
  >
  void addListenerFunctorBcast(CbkT const& cb);
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_VT_PIPE_PIPE_MANAGER_TL_H*/
