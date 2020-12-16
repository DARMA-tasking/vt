/*
//@HEADER
// *****************************************************************************
//
//                                manager.impl.h
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

#if !defined INCLUDED_VT_OBJGROUP_MANAGER_IMPL_H
#define INCLUDED_VT_OBJGROUP_MANAGER_IMPL_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/manager.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/proxy/proxy_bits.h"
#include "vt/objgroup/holder/holder.h"
#include "vt/objgroup/holder/holder_user.h"
#include "vt/objgroup/holder/holder_basic.h"
#include "vt/objgroup/dispatch/dispatch.h"
#include "vt/objgroup/type_registry/registry.h"
#include "vt/registry/auto/auto_registry.h"
#include "vt/collective/collective_alg.h"
#include "vt/messaging/active.h"
#include "vt/runnable/invoke.h"

#include <memory>

namespace vt { namespace objgroup {

// template <typename ObjT>
// ObjGroupManager::ProxyType<ObjT> ObjGroupManager::makeObjGroup() {
//   vtAssert(0, "Rooted makeObjGroup not implemented yet");
//   return ProxyType<ObjT>();
// }

template <typename ObjT, typename... Args>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(Args&&... args) {
  return makeCollective<ObjT>(std::make_unique<ObjT>(std::forward<Args>(args)...));
}

template <typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(ObjT* obj) {
  vtAssert(obj !=  nullptr, "Must be a valid obj pointer");
  auto holder_base = std::make_unique<holder::HolderBasic<ObjT>>(obj);
  return makeCollectiveObj<ObjT>(obj,std::move(holder_base));
}

template <template <typename> class UserPtr, typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(UserPtr<ObjT> obj) {
  auto obj_ptr = *obj;
  vtAssert(obj_ptr !=  nullptr, "Must be a valid obj pointer");
  auto holder_base = std::make_unique<holder::HolderUser<UserPtr,ObjT>>(obj);
  return makeCollectiveObj<ObjT>(obj_ptr,std::move(holder_base));
}

template <typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(std::unique_ptr<ObjT> obj) {
  vtAssert(obj !=  nullptr, "Must be a valid obj pointer");
  auto obj_ptr = obj.get();
  auto holder_base = std::make_unique<holder::Holder<ObjT>>(std::move(obj));
  return makeCollectiveObj<ObjT>(obj_ptr,std::move(holder_base));
}

template <typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollectiveObj(ObjT* obj, HolderBasePtrType holder) {
  auto const obj_type_idx = registry::makeObjIdx<ObjT>();
  auto const obj_ptr = reinterpret_cast<void*>(obj);
  auto const proxy = makeCollectiveImpl(std::move(holder),obj_type_idx,obj_ptr);
  vt_debug_print(
    objgroup, node,
    "makeCollectiveObj: obj_type_idx={}, proxy={:x}\n",
    obj_type_idx, proxy
  );
  regObjProxy<ObjT>(obj, proxy);
  return ProxyType<ObjT>{proxy};
}

template <typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(MakeFnType<ObjT> fn) {
  auto obj = fn();
  return makeCollective<ObjT>(std::move(obj));
}

template <typename ObjT>
void ObjGroupManager::destroyCollective(ProxyType<ObjT> proxy) {
  auto const proxy_bits = proxy.getProxy();
  auto derived_iter = derived_to_bases_.find(proxy_bits);
  vt_debug_print(
    objgroup, node,
    "destroyCollective: proxy={:x}, num bases={}\n", proxy_bits,
    derived_iter != derived_to_bases_.end() ? derived_iter->second.size() : 0
  );
  if (derived_iter != derived_to_bases_.end()) {
    auto base_set = derived_iter->second;
    for (auto&& base_proxy : base_set) {
      auto iter = dispatch_.find(base_proxy);
      if (iter != dispatch_.end()) {
        dispatch_.erase(iter);
      }
    }
    derived_to_bases_.erase(derived_iter);
  }
  auto iter = dispatch_.find(proxy_bits);
  if (iter != dispatch_.end()) {
    auto ptr = iter->second->objPtr();
    auto obj_iter = obj_to_proxy_.find(ptr);
    if (obj_iter != obj_to_proxy_.end()) {
      obj_to_proxy_.erase(obj_iter);
    }
    dispatch_.erase(iter);
  }
  auto obj_iter = objs_.find(proxy_bits);
  if (obj_iter != objs_.end()) {
    objs_.erase(obj_iter);
  }
}

template <typename ObjT>
void ObjGroupManager::regObjProxy(ObjT* obj, ObjGroupProxyType proxy) {
  auto iter = dispatch_.find(proxy);
  vtAssertExpr(iter == dispatch_.end());
  vt_debug_print(
    objgroup, node,
    "regObjProxy: obj={}, proxy={:x}\n",
    print_ptr(obj), proxy
  );
  DispatchBasePtrType b = std::make_unique<dispatch::Dispatch<ObjT>>(proxy,obj);
  dispatch_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(proxy),
    std::forward_as_tuple(std::move(b))
  );
  auto pending_iter = pending_.find(proxy);
  if (pending_iter != pending_.end()) {
    for (auto&& msg : pending_iter->second) {
      theSched()->enqueue([msg]{
        auto const handler = envelopeGetHandler(msg->env);
        auto const epoch = envelopeGetEpoch(msg->env);
        theObjGroup()->dispatch(msg,handler);
        if (epoch != no_epoch) {
          theTerm()->consume(epoch);
        }
      });
    }
    pending_.erase(pending_iter);
  }
}


template <typename ObjT, typename BaseT>
void ObjGroupManager::registerBaseCollective(ProxyType<ObjT> proxy) {
  auto const derived = proxy.getProxy();
  auto base_proxy = derived;
  auto const base_idx = registry::makeObjIdx<BaseT>();
  proxy::ObjGroupProxy::setTypeIdx(base_proxy, base_idx);
  vt_debug_print(
    objgroup, node,
    "ObjGroupManager::registerBaseCollective: derived={:x}, base={:x}\n",
    derived, base_proxy
  );
  derived_to_bases_[derived].insert(base_proxy);
  auto iter = dispatch_.find(derived);
  vtAssertExpr(iter != dispatch_.end());
  if (iter != dispatch_.end()) {
    void* obj_ptr = iter->second->objPtr();
    auto ptr = static_cast<BaseT*>(obj_ptr);
    DispatchBasePtrType b = std::make_unique<dispatch::Dispatch<BaseT>>(base_proxy,ptr);
    dispatch_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(base_proxy),
      std::forward_as_tuple(std::move(b))
    );
  }
}

template <typename ObjT, typename BaseT>
void ObjGroupManager::downcast(ProxyType<ObjT> proxy) {
  auto const derived = proxy.getProxy();
  auto base_proxy = derived;
  auto const base_idx = registry::makeObjIdx<BaseT>();
  proxy::ObjGroupProxy::setTypeIdx(base_proxy, base_idx);
  vt_debug_print(
    objgroup, node,
    "ObjGroupManager::downcast: derived={:x}, base={:x}\n",
    derived, base_proxy
  );
  auto iter = derived_to_bases_[derived].find(base_proxy);
  if (iter == derived_to_bases_[derived].end()) {
    vtAssert(
      false, "Invoke registerBaseCollective on base class before downcast"
    );
  }
}

template <typename ObjT, typename DerivedT>
void ObjGroupManager::upcast(ProxyType<ObjT> proxy) {
}

template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void ObjGroupManager::setTraceName(
  ProxyType<ObjT> proxy, std::string const& name, std::string const& parent
) {
  auto const proxy_bits = proxy.getProxy();
  auto const dest_node = proxy.getNode();
  auto const ctrl = proxy::ObjGroupProxy::getID(proxy_bits);
  auto_registry::setHandlerTraceNameObjGroup<ObjT,MsgT,fn>(ctrl, name, parent);
}

template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void ObjGroupManager::send(ProxyElmType<ObjT> proxy, MsgSharedPtr<MsgT> msg) {
  auto const proxy_bits = proxy.getProxy();
  auto const dest_node = proxy.getNode();
  auto const ctrl = proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT,MsgT,fn>(ctrl);
  vt_debug_print(
    objgroup, node,
    "ObjGroupManager::send: proxy={:x}, node={}, ctrl={:x}, han={:x}\n",
    proxy_bits, dest_node, ctrl, han
  );
  send<MsgT>(msg,han,dest_node);
}

template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void ObjGroupManager::invoke(
  ProxyElmType<ObjT> proxy, messaging::MsgPtrThief<MsgT> msg
) {
  auto const proxy_bits = proxy.getProxy();
  auto const dest_node = proxy.getNode();
  auto const ctrl = proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT, MsgT, fn>(ctrl);

  vt_debug_print(
    objgroup, node,
    "ObjGroupManager::invoke: proxy={:x}, node={}, ctrl={:x}, han={:x}\n",
    proxy_bits, dest_node, ctrl, han
  );

  invoke<MsgT>(msg, han, dest_node);
}

template <typename ObjT, typename Type, Type f, typename... Args>
decltype(auto)
ObjGroupManager::invoke(ProxyElmType<ObjT> proxy, Args&&... args) {
  auto const dest_node = proxy.getNode();
  auto const this_node = theContext()->getNode();

  vtAssert(
    dest_node == this_node,
    fmt::format(
      "Attempting to invoke handler on node:{} instead of node:{}!\n", this_node,
      dest_node
    )
  );

  return runnable::invoke<Type, f>(get(proxy), std::forward<Args>(args)...);
}


template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void ObjGroupManager::broadcast(ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg) {
  auto const proxy_bits = proxy.getProxy();
  auto const ctrl = proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT,MsgT,fn>(ctrl);

  vt_debug_print(
    objgroup, node,
    "ObjGroupManager::broadcast: proxy={:x}, ctrl={:x}, han={:x}\n",
    proxy_bits, ctrl, han
  );

  broadcast<MsgT>(msg, han);
}

template <typename MsgT>
void ObjGroupManager::send(
  MsgSharedPtr<MsgT> msg, HandlerType han, NodeType dest_node
) {
  return objgroup::send(msg,han,dest_node);
}

template <typename MsgT>
void ObjGroupManager::invoke(
  messaging::MsgPtrThief<MsgT> msg, HandlerType han, NodeType dest_node
) {
  objgroup::invoke(msg, han, dest_node);
}

template <typename MsgT>
void ObjGroupManager::broadcast(MsgSharedPtr<MsgT> msg, HandlerType han) {
  return objgroup::broadcast(msg,han);
}

template <typename ObjT, typename MsgT, ActiveTypedFnType<MsgT> *f>
ObjGroupManager::PendingSendType ObjGroupManager::reduce(
  ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg,
  collective::reduce::ReduceStamp const& stamp
) {
  auto const root = 0;
  auto const objgroup = proxy.getProxy();

  auto r = theCollective()->getReducerObjGroup(objgroup);
  return r->template reduce<MsgT,f>(root, msg.get(), stamp);
}

template <typename ObjT>
ObjT* ObjGroupManager::get(ProxyType<ObjT> proxy) {
  auto const this_node = theContext()->getNode();
  return get<ObjT>(ProxyElmType<ObjT>(proxy.getProxy(),this_node));
}

template <typename ObjT>
ObjT* ObjGroupManager::get(ProxyElmType<ObjT> proxy) {
  auto const this_node = theContext()->getNode();
  vtAssert(this_node == proxy.getNode(), "You can only get a local obj");
  auto const proxy_bits = proxy.getProxy();
  auto iter = objs_.find(proxy_bits);
  vtAssert(iter != objs_.end(), "Obj must exist on this node");
  HolderBaseType* holder = iter->second.get();
  auto obj_holder = static_cast<holder::HolderObjBase<ObjT>*>(holder);
  auto obj = obj_holder->get();
  return obj;
}

template <typename ObjT, typename... Args>
void ObjGroupManager::update(ProxyElmType<ObjT> proxy, Args&&... args) {
  auto const this_node = theContext()->getNode();
  vtAssert(this_node == proxy.getNode(), "You can only update a local obj");
  auto const proxy_bits = proxy.getProxy();
  auto iter = objs_.find(proxy_bits);
  vtAssert(iter != objs_.end(), "Obj must exist on this node");
  HolderBaseType* holder = iter->second.get();
  auto obj_holder = static_cast<holder::Holder<ObjT>*>(holder);

  auto const cur_obj_ptr = obj_holder->get();
  auto map_iter = obj_to_proxy_.find(cur_obj_ptr);
  vtAssert(map_iter != obj_to_proxy_.end(), "Object pointer must exist in map");
  obj_to_proxy_.erase(map_iter);

  obj_holder->reset(std::forward<Args>(args)...);

  auto const new_obj_ptr = obj_holder->get();
  obj_to_proxy_[new_obj_ptr] = proxy_bits;
}

template <typename ObjT, typename... Args>
void ObjGroupManager::update(ProxyType<ObjT> proxy, Args&&... args) {
  auto const this_node = theContext()->getNode();
  auto const elm_proxy = ProxyElmType<ObjT>(proxy.getProxy(),this_node);
  return update<ObjT>(elm_proxy,std::forward<Args>(args)...);
}

template <typename ObjT>
typename ObjGroupManager::ProxyType<ObjT> ObjGroupManager::getProxy(ObjT* obj) {
  auto map_iter = obj_to_proxy_.find(obj);
  vtAssert(map_iter != obj_to_proxy_.end(), "Object pointer does not exist");
  return ProxyType<ObjT>(map_iter->second);
}

template <typename ObjT>
typename ObjGroupManager::ProxyElmType<ObjT> ObjGroupManager::proxyElm(ObjT* obj) {
  return getProxy<ObjT>(obj).operator()(theContext()->getNode());
}

}} /* end namespace vt::objgroup */

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_IMPL_H*/
