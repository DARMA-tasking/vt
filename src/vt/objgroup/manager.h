/*
//@HEADER
// ************************************************************************
//
//                           manager.h
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

#if !defined INCLUDED_VT_OBJGROUP_MANAGER_H
#define INCLUDED_VT_OBJGROUP_MANAGER_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/holder/holder.h"
#include "vt/messaging/message/message.h"

#include "vt/registry/auto/auto_registry.h"

#include <memory>
#include <functional>
#include <unordered_map>

namespace vt { namespace objgroup {

struct ObjGroupManager {
  template <typename ObjT>
  using ProxyType       = proxy::Proxy<ObjT>;
  template <typename ObjT>
  using ProxyElmType    = proxy::ProxyElm<ObjT>;
  template <typename ObjT>
  using MakeFnType      = std::function<std::unique_ptr<ObjT>()>;
  template <typename ObjT>
  using HolderPtr       = std::unique_ptr<holder::Holder<ObjT>>;
  template <typename MsgT>
  using DispatchMsgType = std::function<void(HandlerType,MsgSharedPtr<MsgT>)>;
  using DispatchFnType  = DispatchMsgType<ShortMessage>;

  /*
   * Creation of a new object group across the distributed system. For now,
   * these use the default group which includes all the nodes in the
   * communicator
   */
  template <typename ObjT>
  ProxyType<ObjT> makeObjGroup();

  template <typename ObjT>
  ProxyType<ObjT> makeObjGroupCollective(TagType tag = no_tag);
  template <typename ObjT>
  ProxyType<ObjT> makeObjGroupCollective(std::unique_ptr<ObjT> obj);
  template <typename ObjT>
  ProxyType<ObjT> makeObjGroupCollective(MakeFnType<ObjT> fn);

  /*
   * Deletion of a live object group across the system
   */
  template <typename ObjT>
  void rmObjGroupCollective(ProxyType<ObjT> proxy);

  /*
   * Send/broadcast messages to obj group handlers
   */

  template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void send(ProxyElmType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void broadcast(ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  /*
   * Get access to the local instance of a particular object group
   */

  template <typename ObjT>
  void get(ProxyType<ObjT> proxy);

  template <typename ObjT>
  void get(ProxyElmType<ObjT> proxy);

  /*
   * Dispatch to a live obj group pointer with a handler
   */
  void dispatch(MsgSharedPtr<ShortMessage> msg, HandlerType han) {
    auto iter = han_to_proxy_.find(han);
    vtAssertExpr(iter != han_to_proxy_.end());
    auto const proxy = iter->second;
    auto dispatch_iter = dispatch_.find(proxy);
    vtAssertExpr(dispatch_iter != dispatch_.end());
    auto fn = dispatch_iter->second;
    fn(han,msg);
  }

private:
  void regHan(ObjGroupProxyType proxy, HandlerType han) {
    auto iter = han_to_proxy_.find(han);
    vtAssertExpr(iter == han_to_proxy_.end());
    han_to_proxy_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(han),
      std::forward_as_tuple(proxy)
    );
  }

  template <typename ObjT>
  void regDeliver(ObjGroupProxyType proxy) {
    auto iter = dispatch_.find(proxy);
    vtAssertExpr(iter == dispatch_.end());
    auto obj_iter = objs_<ObjT>.find(proxy);
    vtAssertExpr(obj_iter == objs_<ObjT>.end());
    auto obj  = obj_iter->second->get();
    auto deliver_fn = [obj](HandlerType han, MsgSharedPtr<ShortMessage> msg) {
      auto func = auto_registry::getAutoHandlerObjGroup(han);
      (obj->*func)(msg.get());
    };
    dispatch_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(proxy),
      std::forward_as_tuple(deliver_fn)
    );
  }

private:
  template <typename ObjT>
  static std::unordered_map<ObjGroupProxyType,HolderPtr<ObjT>> objs_;
  static ObjGroupIDType cur_obj_id_;
  static std::unordered_map<HandlerType,ObjGroupProxyType> han_to_proxy_;
  static std::unordered_map<ObjGroupProxyType,DispatchFnType> dispatch_;
};

template <typename ObjT>
/*static*/
std::unordered_map<ObjGroupProxyType,ObjGroupManager::HolderPtr<ObjT>>
ObjGroupManager::objs_ = {};

}} /* end namespace vt::objgroup */

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_H*/
