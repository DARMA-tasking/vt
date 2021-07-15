/*
//@HEADER
// *****************************************************************************
//
//                            proxy_objgroup.impl.h
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

#if !defined INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_IMPL_H
#define INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_IMPL_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/manager.h"
#include "vt/collective/collective_alg.h"
#include "vt/collective/reduce/operators/default_op.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"
#include "vt/rdmahandle/manager.h"

namespace vt { namespace objgroup { namespace proxy {

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void Proxy<ObjT>::broadcast(MsgT* inmsg) const {
  MsgPtr<MsgT> msg = promoteMsg(inmsg);
  return broadcast<MsgT,fn>(msg);
}

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void Proxy<ObjT>::broadcast(MsgPtr<MsgT> msg) const {
  auto proxy = Proxy<ObjT>(*this);
  theObjGroup()->broadcast<ObjT,MsgT,fn>(proxy,msg);
}

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void Proxy<ObjT>::broadcastMsg(messaging::MsgPtrThief<MsgT> msg) const {
  auto proxy = Proxy<ObjT>(*this);
  theObjGroup()->broadcast<ObjT,MsgT,fn>(proxy,msg.msg_);
}

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn, typename... Args>
void Proxy<ObjT>::broadcast(Args&&... args) const {
  return broadcastMsg<MsgT,fn>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename ObjT>
template <
  typename OpT, typename MsgPtrT, typename MsgT, ActiveTypedFnType<MsgT> *f
>
typename Proxy<ObjT>::PendingSendType Proxy<ObjT>::reduce(
  MsgPtrT inmsg, Callback<MsgT> cb, ReduceStamp stamp
) const {
  auto proxy = Proxy<ObjT>(*this);
  // Force promoteMsg(Msg*) to avoid special-case overload behavior
  // which is now deprecated. The issue is caused because inmsg can be either
  // Msg* or MsgPtr<Msg> based on call sites.
  MsgPtr<MsgT> msg = promoteMsg(static_cast<MsgT*>(inmsg));
  msg->setCallback(cb);
  return theObjGroup()->reduce<ObjT, MsgT, f>(proxy,msg,stamp);
}

template <typename ObjT>
template <
  typename OpT, typename FunctorT, typename MsgPtrT, typename MsgT,
  ActiveTypedFnType<MsgT> *f
>
typename Proxy<ObjT>::PendingSendType Proxy<ObjT>::reduce(MsgPtrT inmsg, ReduceStamp stamp) const {
  auto proxy = Proxy<ObjT>(*this);
  MsgPtr<MsgT> msg = promoteMsg(static_cast<MsgT*>(inmsg));
  return theObjGroup()->reduce<ObjT, MsgT, f>(proxy,msg,stamp);
}

template <typename ObjT>
template <typename MsgPtrT, typename MsgT, ActiveTypedFnType<MsgT> *f>
typename Proxy<ObjT>::PendingSendType Proxy<ObjT>::reduce(MsgPtrT inmsg, ReduceStamp stamp) const {
  auto proxy = Proxy<ObjT>(*this);
  MsgPtr<MsgT> msg = promoteMsg(inmsg);
  return theObjGroup()->reduce<ObjT, MsgT, f>(proxy,msg,stamp);
}

template <typename ObjT>
ObjT* Proxy<ObjT>::get() const {
  auto proxy = Proxy<ObjT>(*this);
  return theObjGroup()->get<ObjT>(proxy);
}

template <typename ObjT>
ProxyElm<ObjT> Proxy<ObjT>::operator[](NodeType node) const {
  return ProxyElm<ObjT>(proxy_,node);
}

template <typename ObjT>
ProxyElm<ObjT> Proxy<ObjT>::operator()(NodeType node) const {
  return ProxyElm<ObjT>(proxy_,node);
}

template <typename ObjT>
template <typename BaseT>
Proxy<BaseT> Proxy<ObjT>::registerBaseCollective() const {
  static_assert(std::is_base_of<BaseT, ObjT>::value, "BaseT must be base");
  theObjGroup()->registerBaseCollective<ObjT, BaseT>(*this);
  return Proxy<BaseT>(proxy_);
}

template <typename ObjT>
template <typename BaseT>
Proxy<BaseT> Proxy<ObjT>::downcast() const {
  static_assert(std::is_base_of<BaseT, ObjT>::value, "BaseT must be base");
  theObjGroup()->downcast<ObjT, BaseT>(*this);
  return Proxy<BaseT>(proxy_);
}

template <typename ObjT>
template <typename DerivedT>
Proxy<DerivedT> Proxy<ObjT>::upcast() const {
  static_assert(std::is_base_of<ObjT, DerivedT>::value, "Must be base of DerivedT");
  theObjGroup()->upcast<ObjT, DerivedT>(*this);
  return Proxy<DerivedT>(proxy_);
}

template <typename ObjT>
void Proxy<ObjT>::destroyCollective() const {
  return theObjGroup()->destroyCollective<ObjT>(*this);
}

template <typename ObjT>
ObjGroupProxyType Proxy<ObjT>::getProxy() const {
  return proxy_;
}

template <typename ObjT>
template <typename T>
vt::rdma::Handle<T> Proxy<ObjT>::makeHandleRDMA(
  std::size_t count, bool is_uniform
) const {
  return vt::theHandleRDMA()->makeHandleCollectiveObjGroup<
    T, rdma::HandleEnum::StaticSize
  >(*this, count, is_uniform);
}

template <typename ObjT>
template <typename T>
void Proxy<ObjT>::destroyHandleRDMA(vt::rdma::Handle<T> handle) const {
  return vt::theHandleRDMA()->deleteHandleCollectiveObjGroup<
    T, rdma::HandleEnum::StaticSize
  >(handle);
}

template <typename ObjT>
template <typename T>
vt::rdma::HandleSet<T> Proxy<ObjT>::makeHandleSetRDMA(
  int32_t max_elm,
  std::unordered_map<int32_t, std::size_t> const& map,
  bool is_uniform
) const {
  bool dense_start_at_zero = false;
  return vt::theHandleRDMA()->makeHandleSetCollectiveObjGroup<
    T, rdma::HandleEnum::StaticSize
  >(*this, max_elm, map, dense_start_at_zero, is_uniform);
}

template <typename ObjT>
template <typename T>
vt::rdma::HandleSet<T> Proxy<ObjT>::makeHandleSetRDMA(
  int32_t max_elm,
  std::vector<std::size_t> const& vec,
  bool is_uniform
) const {
  int32_t i = 0;
  std::unordered_map<int32_t, std::size_t> map;
  for (auto&& elm : vec) {
    map[i++] = elm;
  }
  bool dense_start_at_zero = true;
  return vt::theHandleRDMA()->makeHandleSetCollectiveObjGroup<
    T, rdma::HandleEnum::StaticSize
  >(*this, max_elm, map, dense_start_at_zero, is_uniform);
}

template <typename ObjT>
template <typename T>
void Proxy<ObjT>::destroyHandleSetRDMA(vt::rdma::HandleSet<T> set) const {
  return vt::theHandleRDMA()->deleteHandleSetCollectiveObjGroup<T>(set);
}

inline DefaultProxyElm Proxy<void>::operator[](NodeType node) const {
  return DefaultProxyElm{node};
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename... Args>
messaging::PendingSend Proxy<void>::broadcast(Args&&... args) const {
  return broadcastMsg<MsgT, f>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
messaging::PendingSend
Proxy<void>::broadcastMsg(messaging::MsgPtrThief<MsgT> msg, TagType tag) const {
  return theMsg()->broadcastMsg<MsgT, f>(msg, true, tag);
}

template <typename OpT, typename FunctorT, typename MsgT, typename... Args>
messaging::PendingSend
Proxy<void>::reduce(NodeType root, Args&&... args) const {
  return reduce<
    OpT,
    FunctorT,
    MsgT,
    &MsgT::template msgHandler<MsgT, OpT, FunctorT>,
    Args...
  >(root, std::forward<Args>(args)...);
}

template <
  typename OpT,
  typename FunctorT,
  typename MsgT,
  ActiveTypedFnType<MsgT>* f,
  typename... Args
>
messaging::PendingSend
Proxy<void>::reduce(NodeType root, Args&&... args) const {
  auto const msg = makeMessage<MsgT>(std::forward<Args>(args)...);
  return reduceMsg<OpT, FunctorT, MsgT, f>(root, msg.get());
}

template <
  typename OpT,
  typename FunctorT,
  typename MsgT
>
messaging::PendingSend
Proxy<void>::reduceMsg(NodeType root, MsgT* const msg) const {
  return reduceMsg<
    OpT,
    FunctorT,
    MsgT,
    &MsgT::template msgHandler<MsgT, OpT, FunctorT>
  >(root, msg);
}

template <
  typename OpT,
  typename FunctorT,
  typename MsgT,
  ActiveTypedFnType<MsgT>* f
>
messaging::PendingSend
Proxy<void>::reduceMsg(NodeType root, MsgT* const msg) const {
  return theCollective()->global()->reduce<OpT, FunctorT>(root, msg);
}

}}} /* end namespace vt::objgroup::proxy */

#endif /*INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_IMPL_H*/
