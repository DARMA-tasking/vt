/*
//@HEADER
// *****************************************************************************
//
//                                  manager.h
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

#if !defined INCLUDED_VT_OBJGROUP_MANAGER_H
#define INCLUDED_VT_OBJGROUP_MANAGER_H

#include "vt/config.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/manager.fwd.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/holder/holder.h"
#include "vt/objgroup/holder/holder_user.h"
#include "vt/objgroup/holder/holder_basic.h"
#include "vt/objgroup/dispatch/dispatch.h"
#include "vt/messaging/message/message.h"
#include "vt/messaging/message/smart_ptr.h"

#include <memory>
#include <functional>
#include <unordered_map>
#include <deque>
#include <vector>
#include <set>

namespace vt { namespace objgroup {

/**
 * \struct ObjGroupManager
 *
 * \brief A core VT component that can create groups with one object per node
 *
 * Create a "object group" with one instance of that group on each node. An
 * instantiated object group has once instance on each node that can be
 * collectively referred to with a single proxy. This proxy allows
 * sends/broadcasts/reduces over the object group.
 *
 * Object groups create a clean isolation of a instance of some functionality
 * that has distributed behavior. Some of the newer VT components are
 * implemented as object groups, such as the load balancers.
 */
struct ObjGroupManager : runtime::component::Component<ObjGroupManager> {
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
  using BaseProxyListType   = std::set<ObjGroupProxyType>;

  /**
   * \internal \brief Construct the ObjGroupManager
   */
  ObjGroupManager() = default;

  std::string name() override { return "ObjGroupManager"; }

  /*
   * Creation of a new object group across the distributed system. For now,
   * these use the default group which includes all the nodes in the
   * communicator
   */

  /**
   * \brief Collectively construct a new object group. Allocates and constructs
   * the object on each node by forwarding constructor arguments.
   *
   * \param[in] args args to pass to the object's constructor on each node
   *
   * \return proxy to the object group
   */
  template <typename ObjT, typename... Args>
  ProxyType<ObjT> makeCollective(Args&&... args);

  /**
   * \brief Collectively construct a new object group from a existing unique
   * pointer to the local object
   *
   * \param[in] obj the std::unique_ptr<ObjT> to the local object
   *
   * \return proxy to the object group
   */
  template <typename ObjT>
  ProxyType<ObjT> makeCollective(std::unique_ptr<ObjT> obj);

  /**
   * \brief Collectively construct a new object group with a callback to provide
   * a unique pointer on each node.
   *
   * \param[in] fn callback function to construct
   *
   * \return proxy to the object group
   */
  template <typename ObjT>
  ProxyType<ObjT> makeCollective(MakeFnType<ObjT> fn);

  /**
   * \brief Collectively construct a new object group from a raw pointer to the
   * object.
   *
   * \warning This overload requires the caller to manage the lifetime of the
   * object. Do not allow the object to be deallocated before the object group
   * is destroyed.
   *
   * \param[in] obj raw pointer to the object
   *
   * \return proxy to the object group
   */
  template <typename ObjT>
  ProxyType<ObjT> makeCollective(ObjT* obj);

  /**
   * \brief Collectively construct a new object group from a smart-pointer-like
   * handle.
   *
   * \param[in] obj the smart-pointer-like handle that the system holds until
   * destruction
   *
   * \return proxy to the object group
   */
  template <template <typename> class UserPtr, typename ObjT>
  ProxyType<ObjT> makeCollective(UserPtr<ObjT> obj);

  /**
   * \brief Collectively destroy an object group across the whole system
   *
   * \param[in] proxy proxy to the object group
   */
  template <typename ObjT>
  void destroyCollective(ProxyType<ObjT> proxy);

  /*
   * Update a live object; swap in a new object with args to delete and
   * reconstruct the object
   */
  /**
   * \brief Locally update the underlying object group instance. Swap the
   * current object group locally for a new instance.
   *
   * \param[in] proxy proxy to the object group
   * \param[in] args args to reconstruct the object group instance
   */
  template <typename ObjT, typename... Args>
  void update(ProxyElmType<ObjT> proxy, Args&&... args);

  /**
   * \brief Collectively update the underlying object group instance. Swap the
   * current object group for a new instance.
   *
   * \param[in] proxy proxy to the object group
   * \param[in] args args to reconstruct the object group instance
   */
  template <typename ObjT, typename... Args>
  void update(ProxyType<ObjT> proxy, Args&&... args);

  /**
   * \internal \brief Send a message to an element of the object group
   *
   * \param[in] proxy proxy to the object group
   * \param[in] msg message to send
   */
  template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void send(ProxyElmType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  /**
   * \internal \brief Broadcast a message to all nodes in object group
   *
   * \param[in] proxy proxy to the object group
   * \param[in] msg message to send
   */
  template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void broadcast(ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  /*
   * Set the tracing name for objgroup
   */
  /**
   * \brief Change the traced name of the object group
   *
   * \param[in] proxy proxy to the object group
   * \param[in] name the new active method name for the object group in traces
   * \param[in] parent the new object function name
   */
  template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void setTraceName(
    ProxyType<ObjT> proxy, std::string const& name, std::string const& parent = ""
  );

  /**
   * \brief Perform a reduction over an objgroup
   *
   * \param[in] proxy proxy to the object group
   * \param[in] msg reduction message
   * \param[in] stamp stamp to identify reduction across nodes
   */
  template <typename ObjT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  void reduce(
    ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg,
    collective::reduce::ReduceStamp const& stamp
  );

  /**
   * \brief Get a pointer to the local objgroup instance
   *
   * \param[in] proxy proxy to the object group
   *
   * \return raw pointer to the object instance on this node
   */
  template <typename ObjT>
  ObjT* get(ProxyType<ObjT> proxy);

  /**
   * \brief Get a pointer to the local objgroup instance
   *
   * \param[in] proxy indexed proxy to the object group (must be the current
   * node)
   *
   * \return raw pointer to the object instance on this node
   */
  template <typename ObjT>
  ObjT* get(ProxyElmType<ObjT> proxy);

  /**
   * \brief Get the proxy from a object instance pointer
   *
   * \param[in] obj the raw pointer to an object
   *
   * \return proxy to the object group
   */
  template <typename ObjT>
  ProxyType<ObjT> getProxy(ObjT* obj);

  /**
   * \brief Get the element proxy from a object instance pointer
   *
   * \param[in] obj the raw pointer to an object
   *
   * \return indexed proxy to the object group
   */
  template <typename ObjT>
  ProxyElmType<ObjT> proxyElm(ObjT* obj);

  /*
   * Dispatch to a live obj group pointer with a handler
   */
  /**
   * \internal \brief Dispatch message to objgroup
   *
   * \param[in] msg the message
   * \param[in] han the handler to invoke
   */
  void dispatch(MsgSharedPtr<ShortMessage> msg, HandlerType han);

  /**
   * \internal \brief Send a message to an objgroup
   *
   * \param[in] msg message to send
   * \param[in] han handler to invoke
   * \param[in] node node to send message
   */
  template <typename MsgT>
  void send(MsgSharedPtr<MsgT> msg, HandlerType han, NodeType node);

  /**
   * \internal \brief Broadcast message to an objgroup
   *
   * \param[in] msg message to broadcast
   * \param[in] han handler to invoke
   */
  template <typename MsgT>
  void broadcast(MsgSharedPtr<MsgT> msg, HandlerType han);

  /**
   * \brief Downcast a proxy to a base class type
   *
   * \param[in] proxy proxy to downcast
   */
  template <typename ObjT, typename BaseT>
  void downcast(ProxyType<ObjT> proxy);

  /**
   * \brief Upcast a proxy to the derived class type
   *
   * \param[in] proxy proxy to upcast
   */
  template <typename ObjT, typename DerivedT>
  void upcast(ProxyType<ObjT> proxy);

  /**
   * \brief Register the base class for a live objgroup
   *
   * If one wants messages delivered to a base class pointer, register the base
   * class.
   *
   * \param[in] proxy the current, derived proxy
   */
  template <typename ObjT, typename BaseT>
  void registerBaseCollective(ProxyType<ObjT> proxy);

  /**
   * \internal \brief Get the proxy
   *
   * \param[in] proxy the proxy to the objgroup
   *
   * \return the proxy
   */
  ObjGroupProxyType getProxy(ObjGroupProxyType proxy);

  friend void scheduleMsg(
    MsgSharedPtr<ShortMessage> msg, HandlerType han, EpochType ep
  );

private:
  /**
   * \internal \brief Untyped system call to make a new collective objgroup
   *
   * \param[in] b the base holder
   * \param[in] idx registered type idx for the objgroup
   * \param[in] obj_ptr type-erased pointer to the object
   *
   * \return a new untyped proxy
   */
  ObjGroupProxyType makeCollectiveImpl(
    HolderBasePtrType b, ObjTypeIdxType idx, void* obj_ptr
  );

  /**
   * \internal \brief Typed system class to make a new collective objgroup
   *
   * \param[in] obj pointer to the object instance
   * \param[in] base_holder the base holder
   *
   * \return the new typed proxy
   */
  template <typename ObjT>
  ProxyType<ObjT> makeCollectiveObj(ObjT* obj, HolderBasePtrType base_holder);

  /**
   * \internal \brief Register a new objgroup with proxy
   *
   * \param[in] obj pointer to the object instance
   * \param[in] proxy the proxy
   */
  template <typename ObjT>
  void regObjProxy(ObjT* obj, ObjGroupProxyType proxy);

private:
  /// The current obj ID, sequential on each node for collective construction
  std::unordered_map<ObjTypeIdxType,ObjGroupIDType> cur_obj_id_;
  /// Function to dispatch to the base class for type-erasure to run handler
  std::unordered_map<ObjGroupProxyType,DispatchBasePtrType> dispatch_;
  /// Type-erased pointers to the objects held on this node
  std::unordered_map<ObjGroupProxyType,HolderBasePtrType> objs_;
  /// Reverse lookup map from an object pointer to the proxy
  std::unordered_map<void*,ObjGroupProxyType> obj_to_proxy_;
  /// Messages that are pending creation for delivery
  std::unordered_map<ObjGroupProxyType,MsgContainerType> pending_;
  /// Map from base class type proxies to registered derived proxy
  std::unordered_map<ObjGroupProxyType,BaseProxyListType> derived_to_bases_;
};

}} /* end namespace vt::objgroup */

#include "vt/objgroup/proxy/proxy_objgroup_elm.impl.h"
#include "vt/objgroup/proxy/proxy_objgroup.impl.h"
#include "vt/objgroup/manager.impl.h"

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_H*/
