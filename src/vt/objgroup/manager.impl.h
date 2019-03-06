/*
//@HEADER
// ************************************************************************
//
//                          manager.impl.h
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

#include <memory>

namespace vt { namespace objgroup {

template <typename ObjT>
ObjGroupManager::ProxyType<ObjT> ObjGroupManager::makeObjGroup() {
  vtAssert(0, "Rooted makeObjGroup not implemented yet");
  return ProxyType<ObjT>();
}

template <typename ObjT, typename... Args>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(Args&&... args) {
  return makeCollective<ObjT>(std::make_unique<ObjT>(std::forward<Args>...));
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
  auto const proxy = makeCollectiveImpl(std::move(holder),obj_type_idx);
  debug_print(
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
  debug_print(
    objgroup, node,
    "destroyCollective: proxy={:x}\n", proxy
  );
  auto iter = dispatch_.find(proxy_bits);
  if (iter != dispatch_.end()) {
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
  debug_print(
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
      work_units_.push_back([msg]{
        auto const handler = envelopeGetHandler(msg->env);
        theObjGroup()->dispatch(msg,handler);
      });
    }
    pending_.erase(pending_iter);
  }
}

template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void ObjGroupManager::send(ProxyElmType<ObjT> proxy, MsgSharedPtr<MsgT> msg) {
  auto const proxy_bits = proxy.getProxy();
  auto const dest_node = proxy.getNode();
  auto const ctrl = proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT,MsgT,fn>(ctrl);
  auto const num_nodes = theContext()->getNumNodes();
  auto const this_node = theContext()->getNode();
  vtAssert(dest_node < num_nodes, "Invalid node (must be < num_nodes)");
  debug_print(
    objgroup, node,
    "ObjGroupManager::send: proxy={:x}, node={}, ctrl={:x}, han={:x}\n",
    proxy_bits, dest_node, ctrl, han
  );
  if (dest_node != this_node) {
    theMsg()->sendMsgAuto<MsgT>(dest_node,han,msg.get(),no_tag);
  } else {
    // Schedule the work of dispatching the message handler for later
    auto umsg = msg.template to<ShortMessage>();
    work_units_.push_back([umsg,han]{ theObjGroup()->dispatch(umsg,han); });
  }
}

template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
void ObjGroupManager::broadcast(ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg) {
  auto const proxy_bits = proxy.getProxy();
  auto const ctrl = proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT,MsgT,fn>(ctrl);
  debug_print(
    objgroup, node,
    "ObjGroupManager::broadcast: proxy={:x}, ctrl={:x}, han={:x}\n",
    proxy_bits, ctrl, han
  );
  theMsg()->broadcastMsgAuto<MsgT>(han,msg.get(),no_tag);
  // Schedule delivery on this node for the objgroup
  auto umsg = msg.template to<ShortMessage>();
  work_units_.push_back([umsg,han]{ theObjGroup()->dispatch(umsg,han); });
}

template <typename ObjT, typename MsgT, ActiveTypedFnType<MsgT> *f>
void ObjGroupManager::reduce(
  ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg, EpochType epoch, TagType tag
) {
  auto const root = 0;
  auto const contrib = 1;
  auto const objgroup = proxy.getProxy();
  theCollective()->reduce<MsgT,f>(
    root, msg.get(), tag, epoch, contrib, no_vrt_proxy, objgroup
  );
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
  auto obj_holder = static_cast<holder::HolderObjBase<ObjT>*>(holder);
  obj_holder->reset(std::forward<Args>(args)...);
}

template <typename ObjT, typename... Args>
void ObjGroupManager::update(ProxyType<ObjT> proxy, Args&&... args) {
  auto const this_node = theContext()->getNode();
  auto const elm_proxy = ProxyElmType<ObjT>(proxy.getProxy(),this_node);
  return update<ObjT>(elm_proxy,std::forward<Args>(args)...);
}

}} /* end namespace vt::objgroup */

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_IMPL_H*/
