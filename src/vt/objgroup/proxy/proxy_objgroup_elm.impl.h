/*
//@HEADER
// *****************************************************************************
//
//                          proxy_objgroup_elm.impl.h
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

#if !defined INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_ELM_IMPL_H
#define INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_ELM_IMPL_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_objgroup_elm.h"
#include "vt/objgroup/manager.h"

namespace vt { namespace objgroup { namespace proxy {

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void ProxyElm<ObjT>::send(MsgT* msg) const {
  return send<MsgT,fn>(promoteMsg(msg));
}

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void ProxyElm<ObjT>::send(MsgSharedPtr<MsgT> msg) const {
  auto proxy = ProxyElm<ObjT>(*this);
  theObjGroup()->send<ObjT,MsgT,fn>(proxy,msg);
}

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
typename ProxyElm<ObjT>::PendingSendType ProxyElm<ObjT>::sendMsg(messaging::MsgPtrThief<MsgT> msg) const {
  auto proxy = ProxyElm<ObjT>(*this);
  return theObjGroup()->send<ObjT,MsgT,fn>(proxy,msg.msg_);
}

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn, typename... Args>
typename ProxyElm<ObjT>::PendingSendType ProxyElm<ObjT>::send(Args&&... args) const {
  return sendMsg<MsgT,fn>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename ObjT>
template <auto f, typename... Params>
typename ProxyElm<ObjT>::PendingSendType
ProxyElm<ObjT>::send(Params&&... params) const {
  using MsgT = typename ObjFuncTraits<decltype(f)>::MsgT;
  if constexpr (std::is_same_v<MsgT, NoMsg>) {
    using Tuple = typename ObjFuncTraits<decltype(f)>::TupleType;
    using SendMsgT = messaging::ParamMsg<Tuple>;
    auto msg = vt::makeMessage<SendMsgT>(std::forward<Params>(params)...);
    auto const ctrl = proxy::ObjGroupProxy::getID(proxy_);
    auto const han = auto_registry::makeAutoHandlerObjGroupParam<
      ObjT, decltype(f), f, SendMsgT
    >(ctrl);
    return theObjGroup()->send(msg, han, node_);
  } else {
    auto msg = makeMessage<MsgT>(std::forward<Params>(params)...);
    return sendMsg<MsgT, f>(msg);
  }

  // Silence nvcc warning (no longer needed for CUDA 11.7 and up)
  return ProxyElm<ObjT>::PendingSendType{std::nullptr_t{}};
}

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> f, typename... Args>
decltype(auto) ProxyElm<ObjT>::invoke(Args&&... args) const {
  auto proxy = ProxyElm<ObjT>(*this);
  auto msg = makeMessage<MsgT>(std::forward<Args>(args)...);
  return theObjGroup()->invoke<ObjT, MsgT, f>(proxy, msg);
}

template <typename ObjT>
template <auto f, typename... Args>
decltype(auto) ProxyElm<ObjT>::invoke(Args&&... args) const {
  auto proxy = ProxyElm<ObjT>(*this);
  using MsgT = typename ObjFuncTraits<decltype(f)>::MsgT;
  if constexpr (std::is_same_v<MsgT, NoMsg>) {
    return theObjGroup()->invoke<ObjT, f>(proxy, std::forward<Args>(args)...);
  } else {
    auto msg = makeMessage<MsgT>(std::forward<Args>(args)...);
    return theObjGroup()->invoke<ObjT, MsgT, f>(proxy, msg);
  }
}

template <typename ObjT>
template <typename SerializerT>
void ProxyElm<ObjT>::serialize(SerializerT& s) {
  s | proxy_ | node_;
}

template <typename ObjT>
template <typename... Args>
void ProxyElm<ObjT>::update(ObjGroupReconstructTagType, Args&&... args) const {
  auto proxy = ProxyElm<ObjT>(*this);
  theObjGroup()->update<ObjT>(proxy,std::forward<Args>(args)...);
}

template <typename ObjT>
ObjT* ProxyElm<ObjT>::get() const {
  auto proxy = ProxyElm<ObjT>(*this);
  return theObjGroup()->get<ObjT>(proxy);
}

inline ProxyElm<void>::ProxyElm(NodeType in_node) : node_{in_node} {}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename... Args>
void ProxyElm<void>::send(Args&&... args) const {
  vt::theMsg()->sendMsg<f>(
    node_, vt::makeMessage<MsgT>(std::forward<Args>(args)...));
}

}}} /* end namespace vt::objgroup::proxy */

#endif /*INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_ELM_IMPL_H*/
