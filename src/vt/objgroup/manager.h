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
#include "vt/objgroup/manager.fwd.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/holder/holder.h"
#include "vt/objgroup/holder/holder_user.h"
#include "vt/objgroup/holder/holder_basic.h"
#include "vt/objgroup/dispatch/dispatch.h"
#include "vt/messaging/message/message.h"

#include <memory>
#include <functional>
#include <unordered_map>
#include <deque>
#include <vector>

namespace vt { namespace objgroup {

struct ObjGroupManager {
  template <typename ObjT>
  using ProxyType           = proxy::Proxy<ObjT>;
  template <typename ObjT>
  using ProxyElmType        = proxy::ProxyElm<ObjT>;
  template <typename ObjT>
  using MakeFnType          = std::function<std::unique_ptr<ObjT>()>;
  using HolderBaseType      = holder::HolderBase;
  using HolderBasePtrType   = std::unique_ptr<HolderBaseType>;
  using DispatchBaseType    = dispatch::DispatchBase;
  using DispatchBasePtrType = std::unique_ptr<DispatchBaseType>;
  using MsgContainerType    = std::vector<MsgSharedPtr<ShortMessage>>;

  ObjGroupManager() = default;

  /*
   * Creation of a new object group across the distributed system. For now,
   * these use the default group which includes all the nodes in the
   * communicator
   */
  template <typename ObjT>
  ProxyType<ObjT> makeObjGroup();

  // Make obj group with the constructor args to the obj
  template <typename ObjT, typename... Args>
  ProxyType<ObjT> makeCollective(Args&&... args);
  // Make obj group with a std::unique_ptr to the obj
  template <typename ObjT>
  ProxyType<ObjT> makeCollective(std::unique_ptr<ObjT> obj);
  // Make obj group with fn for creating the obj
  template <typename ObjT>
  ProxyType<ObjT> makeCollective(MakeFnType<ObjT> fn);
  // Make obj group with non-owning ptr to the object
  template <typename ObjT>
  ProxyType<ObjT> makeCollective(ObjT* obj);
  // Make obj group with specialized smart ptr (e.g., RCP, shared_ptr, etc.)
  template <template <typename> class UserPtr, typename ObjT>
  ProxyType<ObjT> makeCollective(UserPtr<ObjT> obj);

  /*
   * Deletion of a live object group across the system
   */
  template <typename ObjT>
  void destroyCollective(ProxyType<ObjT> proxy);

  /*
   * Update a live object; swap in a new object with args to delete and
   * reconstruct the object
   */
  template <typename ObjT, typename... Args>
  void update(ProxyElmType<ObjT> proxy, Args&&... args);
  template <typename ObjT, typename... Args>
  void update(ProxyType<ObjT> proxy, Args&&... args);

  /*
   * Send/broadcast messages to obj group handlers
   */

  template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void send(ProxyElmType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void broadcast(ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  /*
   * Reduce over an obj group
   */

  template <typename ObjT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduce(
    ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg, EpochType epoch, TagType tag
  );

  /*
   * Get access to the local instance of a particular object group
   */

  template <typename ObjT>
  ObjT* get(ProxyType<ObjT> proxy);
  template <typename ObjT>
  ObjT* get(ProxyElmType<ObjT> proxy);

  /*
   * Dispatch to a live obj group pointer with a handler
   */
  void dispatch(MsgSharedPtr<ShortMessage> msg, HandlerType han);

  /*
   * Run the scheduler to push along postponed events (such as self sends)
   */
  bool scheduler();

private:
  ObjGroupProxyType makeCollectiveImpl(HolderBasePtrType b, ObjTypeIdxType idx);

  template <typename ObjT>
  ProxyType<ObjT> makeCollectiveObj(ObjT* obj, HolderBasePtrType base_holder);

  template <typename ObjT>
  void regObjProxy(ObjT* obj, ObjGroupProxyType proxy);

private:
  // The current obj ID, sequential on each node for collective construction
  std::unordered_map<ObjTypeIdxType,ObjGroupIDType> cur_obj_id_;
  // Function to dispatch to the base class for type-erasure to run handler
  std::unordered_map<ObjGroupProxyType,DispatchBasePtrType> dispatch_;
  // Type-erased pointers to the objects held on this node
  std::unordered_map<ObjGroupProxyType,HolderBasePtrType> objs_;
  // Work units to be scheduled
  std::deque<ActionType> work_units_;
  // Messages that are pending creation for delivery
  std::unordered_map<ObjGroupProxyType,MsgContainerType> pending_;
};

}} /* end namespace vt::objgroup */

#include "vt/objgroup/manager.impl.h"
#include "vt/objgroup/proxy/proxy_objgroup_elm.impl.h"
#include "vt/objgroup/proxy/proxy_objgroup.impl.h"

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_H*/
