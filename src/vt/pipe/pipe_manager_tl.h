/*
//@HEADER
// ************************************************************************
//
//                          pipe_manager_tl.h
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

#if !defined INCLUDED_PIPE_PIPE_MANAGER_TL_H
#define INCLUDED_PIPE_PIPE_MANAGER_TL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager_base.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/vrt/proxy/collection_proxy.h"

namespace vt { namespace pipe {

struct PipeManagerTL : virtual PipeManagerBase {
  template <typename ColT, typename MsgT>
  using ColHanType = vrt::collection::ActiveColTypedFnType<MsgT,ColT>;

  template <typename ColT>
  using ColProxyType = CollectionProxy<ColT,typename ColT::IndexType>;

  using CallbackType = callback::cbunion::CallbackRawBaseSingle;

  template <typename MsgT>
  using CallbackMsgType = callback::cbunion::CallbackTyped<MsgT>;

  template <typename MsgT>
  using FnType = ActiveTypedFnType<MsgT>;

  template <typename MsgT>
  using DefType = CallbackMsgType<MsgT>;

  using V = signal::SigVoidType;

  /*
   *  Untyped variants of callbacks: uses union to dispatch
   */


  template <typename T, FnType<T>* f, typename CbkT = DefType<T>>
  CbkT makeCallbackSingleSendT(NodeType const& node);

  // Single active message function-handler
  template <typename T, FnType<T>* f, typename CbkT = DefType<T>>
  CbkT makeCallbackSingleSend(NodeType const& node);

  template <typename T, FnType<T>* f, typename CbkT = DefType<T>>
  CbkT makeCallbackSingleBcast(bool const& inc);

  // Single active message functor-handler
  template <
    typename FunctorT,
    typename T = typename util::FunctorExtractor<FunctorT>::MessageType,
    typename CbkT = DefType<T>
  >
  CbkT makeCallbackFunctorSend(NodeType const& node);

  template <
    typename FunctorT,
    typename T = typename util::FunctorExtractor<FunctorT>::MessageType,
    typename CbkT = DefType<T>
  >
  CbkT makeCallbackFunctorBcast(bool const& inc);

  // Single active message functor-handler void param
  template <typename FunctorT, typename CbkT = DefType<V>>
  CbkT makeCallbackFunctorSendVoid(NodeType const& node);

  template <typename FunctorT, typename CbkT = DefType<V>>
  CbkT makeCallbackFunctorBcastVoid(bool const& inc);

  // Single active message anon func-handler
  template <typename CbkT = DefType<V>>
  CbkT makeCallbackSingleAnonVoid(FuncVoidType fn);

  template <typename T, typename CbkT = DefType<T>>
  CbkT makeCallbackSingleAnon(FuncMsgType<T> fn);

  template <typename T, typename U, typename CbkT = DefType<T>>
  CbkT makeCallbackSingleAnon(U* u, FuncMsgCtxType<T, U> fn);

  // Single active message collection proxy send
  template <
    typename ColT, typename T, ColHanType<ColT,T>* f, typename CbkT = DefType<T>
  >
  CbkT makeCallbackSingleProxySend(typename ColT::ProxyType proxy);

  // Single active message collection proxy bcast
  template <
    typename ColT, typename T, ColHanType<ColT,T>* f, typename CbkT = DefType<T>
  >
  CbkT makeCallbackSingleProxyBcast(ColProxyType<ColT> proxy);

  // Single active message collection proxy bcast direct
  template <
    typename ColT, typename T, ColHanType<ColT,T>* f, typename CbkT = DefType<T>
  >
  CbkT makeCallbackSingleProxyBcastDirect(ColProxyType<ColT> proxy);

  // Multi-staged callback
  template <typename=void>
  CallbackType makeCallback();

  template <typename T>
  CallbackMsgType<T> makeCallbackTyped();

  template <typename T, ActiveTypedFnType<T>* f, typename CbkT = DefType<T>>
  void addListener(CbkT const& cb, NodeType const& node);

  template <typename T, ActiveTypedFnType<T>* f, typename CbkT = DefType<T>>
  void addListenerBcast(CbkT const& cb, bool const& inc);

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
  void addListenerFunctorBcast(CbkT const& cb, bool const& inc);
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TL_H*/
