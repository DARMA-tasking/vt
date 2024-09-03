/*
//@HEADER
// *****************************************************************************
//
//                                manager.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/objgroup/type_registry/registry.h"
#include "vt/registry/auto/auto_registry.h"
#include "vt/collective/collective_alg.h"
#include "vt/messaging/active.h"
#include "vt/elm/elm_id_bits.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/collective/reduce/allreduce/rabenseifner.h"
#include "vt/collective/reduce/allreduce/recursive_doubling.h"
#include "vt/collective/reduce/allreduce/type.h"
#include "vt/collective/reduce/allreduce/helpers.h"
#include <utility>
#include <array>

#include <memory>

namespace vt { namespace objgroup {

// template <typename ObjT>
// ObjGroupManager::ProxyType<ObjT> ObjGroupManager::makeObjGroup() {
//   vtAssert(0, "Rooted makeObjGroup not implemented yet");
//   return ProxyType<ObjT>();
// }

template <typename ObjT, typename... Args>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(std::string const& label, Args&&... args) {
  return makeCollective<ObjT>(std::make_unique<ObjT>(std::forward<Args>(args)...), label);
}

template <typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(ObjT* obj, std::string const& label) {
  vtAssert(obj !=  nullptr, "Must be a valid obj pointer");
  auto holder_base = std::make_unique<holder::HolderBasic<ObjT>>(obj);
  return makeCollectiveObj<ObjT>(label, obj, std::move(holder_base));
}

template <template <typename> class UserPtr, typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(UserPtr<ObjT> obj, std::string const& label) {
  auto obj_ptr = *obj;
  vtAssert(obj_ptr !=  nullptr, "Must be a valid obj pointer");
  auto holder_base = std::make_unique<holder::HolderUser<UserPtr,ObjT>>(obj);
  return makeCollectiveObj<ObjT>(label, obj_ptr, std::move(holder_base));
}

template <typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(std::unique_ptr<ObjT> obj, std::string const& label) {
  vtAssert(obj !=  nullptr, "Must be a valid obj pointer");
  auto obj_ptr = obj.get();
  auto holder_base = std::make_unique<holder::Holder<ObjT>>(std::move(obj));
  return makeCollectiveObj<ObjT>(label, obj_ptr, std::move(holder_base));
}

template <typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollectiveObj(std::string const& label, ObjT* obj, HolderBasePtrType holder) {
  auto const obj_ptr = reinterpret_cast<std::byte*>(obj);
  auto const proxy = makeCollectiveImpl(label, std::move(holder), obj_ptr);
  auto iter = objs_.find(proxy);
  vtAssert(iter != objs_.end(), "Obj must exist on this node");
  HolderBaseType* h = iter->second.get();
  h->setElmID(getNextElm(proxy));
  vt_debug_print(
    terse, objgroup,
    "makeCollectiveObj: proxy={:x}\n",
    proxy
  );
  regObjProxy<ObjT>(obj, proxy);
  return ProxyType<ObjT>{proxy};
}

template <typename ObjT>
ObjGroupManager::ProxyType<ObjT>
ObjGroupManager::makeCollective(MakeFnType<ObjT> fn, std::string const& label) {
  auto obj = fn();
  return makeCollective<ObjT>(std::move(obj), label);
}

template <typename ObjT>
void ObjGroupManager::destroyCollective(ProxyType<ObjT> proxy) {
  auto const proxy_bits = proxy.getProxy();
  auto obj_iter = objs_.find(proxy_bits);
  if (obj_iter != objs_.end()) {
    objs_.erase(obj_iter);
  }

  auto label_iter = labels_.find(proxy_bits);
  if (label_iter != labels_.end()) {
    labels_.erase(label_iter);
  }
}

template <typename ObjT>
void ObjGroupManager::regObjProxy(ObjT* obj, ObjGroupProxyType proxy) {
  vt_debug_print(
    normal, objgroup,
    "regObjProxy: obj={}, proxy={:x}\n",
    print_ptr(obj), proxy
  );
  auto pending_iter = pending_.find(proxy);
  if (pending_iter != pending_.end()) {
    for (auto&& pending : pending_iter->second) {
      pending();
    }
    pending_.erase(pending_iter);
  }
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
ObjGroupManager::PendingSendType ObjGroupManager::send(ProxyElmType<ObjT> proxy, MsgSharedPtr<MsgT> msg) {
  auto const proxy_bits = proxy.getProxy();
  auto const dest_node = proxy.getNode();
  auto const ctrl = proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT,MsgT,fn>(ctrl);

  // set bit so it isn't recorded as it routes through bare
  // handlers
  envelopeSetCommLBDataRecordedAboveBareHandler(msg->env, true);

  if (theContext()->getTask() != nullptr) {
    auto dest_elm_id = elm::ElmIDBits::createObjGroup(proxy_bits, dest_node);
    auto const num_bytes = serialization::MsgSizer<MsgT>::get(msg.get());
    theContext()->getTask()->send(dest_elm_id, num_bytes);
  }

  vt_debug_print(
    terse, objgroup,
    "ObjGroupManager::send: proxy={:x}, node={}, ctrl={:x}, han={:x}\n",
    proxy_bits, dest_node, ctrl, han
  );

  return send<MsgT>(msg,han,dest_node);
}

template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
decltype(auto) ObjGroupManager::invoke(
  ProxyElmType<ObjT> proxy, messaging::MsgPtrThief<MsgT> msg
) {
  auto const proxy_bits = proxy.getProxy();
  auto const dest_node = proxy.getNode();
  auto const ctrl = proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT, MsgT, fn>(ctrl);

  vt_debug_print(
    terse, objgroup,
    "ObjGroupManager::invoke: proxy={:x}, node={}, ctrl={:x}, han={:x}\n",
    proxy_bits, dest_node, ctrl, han
  );

  auto& msg_ptr = msg.msg_;
  return invoke<ObjT, MsgT, fn>(msg_ptr, han, dest_node);
}

template <typename ObjT, auto f, typename... Args>
decltype(auto)
ObjGroupManager::invoke(ProxyElmType<ObjT> proxy, Args&&... args) {
  auto const dest_node = proxy.getNode();
  auto const this_node = theContext()->getNode();

  vtAssert(
    dest_node == this_node,
    fmt::format(
      "Attempting to invoke handler on node:{} instead of node:{}!\n",
      this_node, dest_node));

  return runnable::makeRunnableVoid(false, uninitialized_handler, this_node)
    .withObjGroup(get(proxy))
    .runLambda(f, get(proxy), std::forward<Args>(args)...);
}


template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
ObjGroupManager::PendingSendType ObjGroupManager::broadcast(ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg) {
  auto const proxy_bits = proxy.getProxy();
  auto const ctrl = proxy::ObjGroupProxy::getID(proxy_bits);
  auto const han = auto_registry::makeAutoHandlerObjGroup<ObjT,MsgT,fn>(ctrl);

  vt_debug_print(
    terse, objgroup,
    "ObjGroupManager::broadcast: proxy={:x}, ctrl={:x}, han={:x}\n",
    proxy_bits, ctrl, han
  );

  return broadcast<MsgT>(msg, han);
}

template <typename MsgT>
ObjGroupManager::PendingSendType ObjGroupManager::send(
  MsgSharedPtr<MsgT> msg, HandlerType han, NodeType dest_node
) {
  return objgroup::send(msg,han,dest_node);
}

template <typename ObjT, typename MsgT, auto f>
decltype(auto) ObjGroupManager::invoke(
  messaging::MsgSharedPtr<MsgT> msg, HandlerType han, NodeType dest_node
) {
  return objgroup::invoke<ObjT, MsgT, f>(msg, han, dest_node);
}

template <typename MsgT>
ObjGroupManager::PendingSendType ObjGroupManager::broadcast(MsgSharedPtr<MsgT> msg, HandlerType han) {
  return objgroup::broadcast(msg,han);
}

template <auto f, typename Reducer, typename ObjT, typename... Args>
ObjGroupManager::PendingSendType ObjGroupManager::allreduce(
  ProxyType<ObjT> proxy, Args&&... data) {
  using namespace vt::collective::reduce::allreduce;

  auto const this_node = vt::theContext()->getNode();
  size_t id = 0;

  proxy::Proxy<Reducer> grp_proxy = {};

  auto& reducers = Reducer::type_ == ReducerType::Rabenseifner ?
    reducers_rabenseifner_ :
    reducers_recursive_doubling_;
  auto const key = std::make_tuple(
    proxy.getProxy(), std::type_index(typeid(typename Reducer::Data)),
    std::type_index(typeid(typename Reducer::ReduceOp))
  );
  if (reducers.find(key) != reducers.end()) {
    vt_debug_print(
      verbose, allreduce, "Found reducer (type: {}) for proxy {:x}",
      TypeToString(Reducer::type_), proxy.getProxy()
    );

    auto* obj =
      reinterpret_cast<Reducer*>(objs_.at(reducers.at(key))->getPtr());
    id = obj->generateNewId();
    obj->initialize(id, std::forward<Args>(data)...);
    grp_proxy = obj->proxy_;
  } else {
    vt_debug_print(
      verbose, allreduce, "Creating reducer (type: {}) for proxy {:x}",
      TypeToString(Reducer::type_), proxy.getProxy()
    );

    grp_proxy = vt::theObjGroup()->makeCollective<Reducer>(
      TypeToString(Reducer::type_), std::forward<Args>(data)...
    );
    grp_proxy[this_node].get()->proxy_ = grp_proxy;
    reducers[key] = grp_proxy.getProxy();
    id = grp_proxy[this_node].get()->id_ - 1;
  }

  auto cb = theCB()->makeSend<f>(proxy[this_node]);
  grp_proxy[this_node].get()->setFinalHandler(cb);

  return PendingSendType{
    theTerm()->getEpoch(),
    [=] { grp_proxy[this_node].template invoke<&Reducer::allreduce>(id); }
  };
}

template <auto f, typename ObjT, template <typename Arg> class Op, typename DataT, typename... Args>
ObjGroupManager::PendingSendType
ObjGroupManager::allreduce(ProxyType<ObjT> proxy, Args&&... data) {
  using namespace collective::reduce::allreduce;

  auto const this_node = vt::theContext()->getNode();
  if (theContext()->getNumNodes() < 2) {
    return PendingSendType{theTerm()->getEpoch(), [&] {
      proxy[this_node].template invoke<f>(std::forward<Args>(data)...);
    }};
  }

  // using Obj = typename FuncTraits<decltype(f)>::ObjT;
  // auto cb = theCB()->makeSend<f>(proxy[this_node]);

  auto const payload_size =
    DataHandler<remove_cvref<DataT>>::size(std::forward<Args>(data)...);

  if (payload_size < 2048) {
    using Reducer = RecursiveDoubling<DataT, Op, f>;
    return allreduce<f, Reducer>(proxy, std::forward<Args>(data)...);
  } else {
    using Reducer = Rabenseifner<ObjgroupAllreduceT, DataT, Op, f>;
    return allreduce<f, Reducer>(proxy, std::forward<Args>(data)...);
  }

  // Silence nvcc warning
  return PendingSendType{nullptr};
}

template <typename ObjT, typename MsgT, ActiveTypedFnType<MsgT> *f>
ObjGroupManager::PendingSendType ObjGroupManager::reduce(
  ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg,
  collective::reduce::ReduceStamp const& stamp
) {
  auto const root = 0;
  auto const objgroup = proxy.getProxy();

  auto r = theCollective()->getReducerObjGroup(objgroup);
  return r->template reduce<f>(root, msg.get(), stamp);
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
  return reinterpret_cast<ObjT*>(holder->getPtr());
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

  auto const cur_obj_ptr = reinterpret_cast<std::byte*>(obj_holder->get());
  auto map_iter = obj_to_proxy_.find(cur_obj_ptr);
  vtAssert(map_iter != obj_to_proxy_.end(), "Object pointer must exist in map");
  obj_to_proxy_.erase(map_iter);

  obj_holder->reset(std::forward<Args>(args)...);

  auto const new_obj_ptr = reinterpret_cast<std::byte*>(obj_holder->get());
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
  auto map_iter = obj_to_proxy_.find(reinterpret_cast<std::byte*>(obj));
  vtAssert(map_iter != obj_to_proxy_.end(), "Object pointer does not exist");
  return ProxyType<ObjT>(map_iter->second);
}

template <typename ObjT>
typename ObjGroupManager::ProxyElmType<ObjT> ObjGroupManager::proxyElm(ObjT* obj) {
  return getProxy<ObjT>(obj).operator()(theContext()->getNode());
}

template <typename ObjT>
std::string ObjGroupManager::getLabel(ObjGroupManager::ProxyType<ObjT> proxy) const {
  auto const proxy_bits = proxy.getProxy();
  auto const iter = labels_.find(proxy_bits);
  vtAssert(iter != labels_.end(), "Obj label does not exist");
  return iter->second;
}

}} /* end namespace vt::objgroup */

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_IMPL_H*/
