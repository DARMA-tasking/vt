/*
//@HEADER
// ************************************************************************
//
//                      proxy_objgroup.impl.h
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

#if !defined INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_IMPL_H
#define INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_IMPL_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/manager.h"
#include "vt/collective/collective_alg.h"
#include "vt/collective/reduce/operators/default_op.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"

namespace vt { namespace objgroup { namespace proxy {

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void Proxy<ObjT>::broadcast(MsgT* msg) const {
  return broadcast<MsgT,fn>(promoteMsg(msg));
}

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void Proxy<ObjT>::broadcast(MsgSharedPtr<MsgT> msg) const {
  auto proxy = Proxy<ObjT>(*this);
  theObjGroup()->broadcast<ObjT,MsgT,fn>(proxy,msg);
}

template <typename ObjT>
template <typename MsgT, ActiveObjType<MsgT, ObjT> fn, typename... Args>
void Proxy<ObjT>::broadcast(Args&&... args) const {
  return broadcast<MsgT,fn>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename ObjT>
template <
  typename OpT, typename MsgPtrT, typename MsgT, ActiveTypedFnType<MsgT> *f
>
EpochType Proxy<ObjT>::reduce(
  MsgPtrT inmsg, Callback<MsgT> cb, EpochType epoch, TagType tag
) const {
  auto proxy = Proxy<ObjT>(*this);
  auto msg = promoteMsg(inmsg);
  msg->setCallback(cb);
  return theObjGroup()->reduce<ObjT, MsgT, f>(proxy,msg,epoch,tag);
}

template <typename ObjT>
template <
  typename OpT, typename FunctorT, typename MsgPtrT, typename MsgT,
  ActiveTypedFnType<MsgT> *f
>
EpochType Proxy<ObjT>::reduce(
  MsgPtrT inmsg, EpochType epoch, TagType tag
) const {
  auto proxy = Proxy<ObjT>(*this);
  auto msg = promoteMsg(inmsg);
  return theObjGroup()->reduce<ObjT, MsgT, f>(proxy,msg,epoch,tag);
}

template <typename ObjT>
template <typename MsgPtrT, typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Proxy<ObjT>::reduce(
  MsgPtrT inmsg, EpochType epoch, TagType tag
) const {
  auto proxy = Proxy<ObjT>(*this);
  auto msg = promoteMsg(inmsg);
  return theObjGroup()->reduce<ObjT, MsgT, f>(proxy,msg,epoch,tag);
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
ObjGroupProxyType Proxy<ObjT>::getProxy() const {
  return proxy_;
  //return theObjGroup()->getProxy(proxy_);
}

}}} /* end namespace vt::objgroup::proxy */

#endif /*INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_IMPL_H*/
