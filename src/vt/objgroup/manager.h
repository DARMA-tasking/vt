/*
//@HEADER
// *****************************************************************************
//
//                                  manager.h
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

#if !defined INCLUDED_VT_OBJGROUP_MANAGER_H
#define INCLUDED_VT_OBJGROUP_MANAGER_H

#include "vt/config.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/utils/static_checks/function_ret_check.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/manager.fwd.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/objgroup/holder/holder.h"
#include "vt/objgroup/holder/holder_user.h"
#include "vt/objgroup/holder/holder_basic.h"
#include "vt/messaging/message/message.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/messaging/pending_send.h"
#include "vt/elm/elm_id.h"
#include "vt/utils/fntraits/fntraits.h"

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>

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
  using PendingSendType     = messaging::PendingSend;

public:
  /**
   * \internal \brief Construct the ObjGroupManager
   */
  ObjGroupManager() = default;

  std::string name() override { return "ObjGroupManager"; }

  void startup() override;

  /*
   * Creation of a new object group across the distributed system. For now,
   * these use the default group which includes all the nodes in the
   * communicator
   */

  /**
   * \brief Construct a special proxy instance that allows sending, broadcasting
   * and reducing without actual object group.
   *
   * \return proxy to the object group
   */
  proxy::DefaultProxyType getDefault() const;

  /**
   * \brief Collectively construct a new object group. Allocates and constructs
   * the object on each node by forwarding constructor arguments.
   *
   * \param[in] label object group label
   * \param[in] args args to pass to the object's constructor on each node
   *
   * \return proxy to the object group
   */
  template <typename ObjT, typename... Args>
  ProxyType<ObjT> makeCollective(std::string const& label, Args&&... args);

  /**
   * \brief Collectively construct a new object group from a existing unique
   * pointer to the local object
   *
   * \param[in] obj the std::unique_ptr<ObjT> to the local object
   * \param[in] label object group label
   *
   * \return proxy to the object group
   */
  template <typename ObjT>
  ProxyType<ObjT> makeCollective(std::unique_ptr<ObjT> obj, std::string const& label = {});

  /**
   * \brief Collectively construct a new object group with a callback to provide
   * a unique pointer on each node.
   *
   * \param[in] fn callback function to construct
   * \param[in] label object group label
   *
   * \return proxy to the object group
   */
  template <typename ObjT>
  ProxyType<ObjT> makeCollective(MakeFnType<ObjT> fn, std::string const& label = {});

  /**
   * \brief Collectively construct a new object group from a raw pointer to the
   * object.
   *
   * \warning This overload requires the caller to manage the lifetime of the
   * object. Do not allow the object to be deallocated before the object group
   * is destroyed.
   *
   * \param[in] obj raw pointer to the object
   * \param[in] label object group label
   *
   * \return proxy to the object group
   */
  template <typename ObjT>
  ProxyType<ObjT> makeCollective(ObjT* obj, std::string const& label = {});

  /**
   * \brief Collectively construct a new object group from a smart-pointer-like
   * handle.
   *
   * \param[in] obj the smart-pointer-like handle that the system holds until
   * destruction
   * \param[in] label object group label
   *
   * \return proxy to the object group
   */
  template <template <typename> class UserPtr, typename ObjT>
  ProxyType<ObjT> makeCollective(UserPtr<ObjT> obj, std::string const& label = {});

  /**
   * \brief Collectively destroy an object group across the whole system
   *
   * \param[in] proxy proxy to the object group
   */
  template <typename ObjT>
  void destroyCollective(ProxyType<ObjT> proxy);

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
  PendingSendType send(ProxyElmType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  /**
   * \internal \brief Send a message to an element of the object group
   *
   * \param[in] proxy proxy to the object group
   * \param[in] msg message to send
   */
  template <auto fn>
  PendingSendType send(
    ProxyElmType<typename ObjFuncTraits<decltype(fn)>::ObjT> proxy,
    MsgSharedPtr<typename ObjFuncTraits<decltype(fn)>::MsgT> msg
  ) {
    using ObjType = typename ObjFuncTraits<decltype(fn)>::ObjT;
    using MsgType = typename ObjFuncTraits<decltype(fn)>::MsgT;
    return send<ObjType, MsgType, fn>(proxy, msg);
  }

  /**
   * \internal \brief Invoke message handler on an element of the object group
   * The message handler will be invoked inline without going through scheduler
   *
   * \param[in] proxy proxy to the object group
   * \param[in] msg message
   */
  template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  decltype(auto) invoke(ProxyElmType<ObjT> proxy, messaging::MsgPtrThief<MsgT> msg);

  /**
   * \internal \brief Invoke function 'f' on an element of the object group
   * The function will be invoked inline without going through scheduler
   *
   * \param[in] proxy proxy to the object group
   * \param[in] args function arguments
   */
  template <typename ObjT, auto f, typename... Args>
  decltype(auto) invoke(ProxyElmType<ObjT> proxy, Args&&... args);

  /**
   * \internal \brief Broadcast a message to all nodes in object group
   *
   * \param[in] proxy proxy to the object group
   * \param[in] msg message to broadcast
   */
  template <typename ObjT, typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  PendingSendType broadcast(ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg);

  /**
   * \internal \brief Broadcast a message to all nodes in object group
   *
   * \param[in] proxy proxy to the object group
   * \param[in] msg message to broadcast
   */
  template <auto fn>
  PendingSendType broadcast(
    ProxyType<typename ObjFuncTraits<decltype(fn)>::ObjT> proxy,
    MsgSharedPtr<typename ObjFuncTraits<decltype(fn)>::MsgT> msg
  ) {
    using ObjType = typename ObjFuncTraits<decltype(fn)>::ObjT;
    using MsgType = typename ObjFuncTraits<decltype(fn)>::MsgT;
    return broadcast<ObjType, MsgType, fn>(proxy, msg);
  }

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
   *
   * \return the PendingSend corresponding to the reduce
   */
  template <typename ObjT, typename MsgT, ActiveTypedFnType<MsgT> *f>
  PendingSendType reduce(
    ProxyType<ObjT> proxy, MsgSharedPtr<MsgT> msg,
    collective::reduce::ReduceStamp const& stamp
  );

  /**
   * \brief Perform a reduction over an objgroup
   *
   * \param[in] proxy proxy to the object group
   * \param[in] msg reduction message
   * \param[in] stamp stamp to identify reduction across nodes
   *
   * \return the PendingSend corresponding to the reduce
   */
  template <typename ObjT, auto f>
  PendingSendType reduce(
    ProxyType<ObjT> proxy,
    messaging::MsgPtrThief<typename FuncTraits<decltype(f)>::MsgT> msg,
    collective::reduce::ReduceStamp const& stamp
  ) {
    using MsgT = typename FuncTraits<decltype(f)>::MsgT;
    return reduce<ObjT, MsgT, f>(proxy, msg, stamp);
  }

  /**
   * \brief Get a pointer to the local objgroup instance. Returns null if the
   * object doesn't exist.
   *
   * \param[in] proxy proxy to the object group
   *
   * \return raw pointer to the object instance on this node
   */
  template <typename ObjT>
  ObjT* get(ProxyType<ObjT> proxy);

  /**
   * \brief Get a pointer to the local objgroup instance. Returns null if the
   * object doesn't exist.
   *
   * \param[in] proxy indexed proxy to the object group (must be the current
   * node)
   *
   * \return raw pointer to the object instance on this node
   */
  template <typename ObjT>
  ObjT* get(ProxyElmType<ObjT> proxy);

  /**
   * \brief Get the proxy from a object instance pointer. Assert that object
   * pointer is part of a valid object group.
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

  /**
   * \brief Get object group label
   *
   * \param[in] proxy indexed proxy to the object group (must be the current
   * node)
   *
   * \return label of the Object Group
   */
  template <typename ObjT>
  std::string getLabel(ProxyType<ObjT> proxy) const;

  /**
   * \internal \brief Send a message to an objgroup
   *
   * \param[in] msg message to send
   * \param[in] han handler to invoke
   * \param[in] node node to send message
   */
  template <typename MsgT>
  PendingSendType send(MsgSharedPtr<MsgT> msg, HandlerType han, NodeType node);

  /**
   * \internal \brief Invoke a message handler on an objgroup
   * The message handler will be invoked inline without going through scheduler
   *
   * \param[in] msg message
   * \param[in] han handler to invoke
   * \param[in] node node to invoke the handler on
   */
  template <typename ObjT, typename MsgT, auto f>
  decltype(auto) invoke(
    messaging::MsgSharedPtr<MsgT> msg, HandlerType han, NodeType node
  );

  /**
   * \internal \brief Broadcast message to an objgroup
   *
   * \param[in] msg message to broadcast
   * \param[in] han handler to invoke
   */
  template <typename MsgT>
  PendingSendType broadcast(MsgSharedPtr<MsgT> msg, HandlerType han);

  /**
   * \internal \brief Get the proxy, identity function
   *
   * \param[in] proxy the proxy to the objgroup
   *
   * \return the proxy
   */
  ObjGroupProxyType getProxy(ObjGroupProxyType proxy);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | cur_obj_id_
      | objs_
      | obj_to_proxy_
      | pending_
      | labels_;
  }

  // Friend function to access the holder without including this header file
  friend holder::HolderBase* detail::getHolderBase(HandlerType handler);
  friend std::unordered_map<ObjGroupProxyType, HolderBasePtrType>& getObjs();
  friend std::unordered_map<ObjGroupProxyType, std::vector<ActionType>>& getPending();

private:
  /**
   * \internal \brief Untyped system call to make a new collective objgroup
   *
   * \param[in] label object group label
   * \param[in] b the base holder
   * \param[in] obj_ptr type-erased pointer to the object
   *
   * \return a new untyped proxy
   */
  ObjGroupProxyType makeCollectiveImpl(
    std::string const& label, HolderBasePtrType b, void* obj_ptr
  );

  /**
   * \internal \brief Typed system class to make a new collective objgroup
   *
   * \param[in] label
   * \param[in] obj pointer to the object instance
   * \param[in] base_holder the base holder
   *
   * \return the new typed proxy
   */
  template <typename ObjT>
  ProxyType<ObjT> makeCollectiveObj(
    std::string const& label, ObjT* obj, HolderBasePtrType base_holder
  );

  /**
   * \internal \brief Register a new objgroup with proxy
   *
   * \param[in] obj pointer to the object instance
   * \param[in] proxy the proxy
   */
  template <typename ObjT>
  void regObjProxy(ObjT* obj, ObjGroupProxyType proxy);

  /**
   * \internal \brief Get the holder for an objgroup from a handler
   *
   * \param[in] han the handler
   *
   * \return the base holder
   */
  HolderBaseType* getHolderBase(HandlerType han);

  /**
   * \internal \brief Get the next element ID from \c NodeLBData
   *
   * \param[in] proxy the objgroup proxy
   *
   * \return the next element ID
   */
  elm::ElementIDStruct getNextElm(ObjGroupProxyType proxy);

private:
  /// The current obj ID, sequential on each node for collective construction
  ObjGroupIDType cur_obj_id_ = fst_obj_group_id;
  /// Type-erased pointers to the objects held on this node
  std::unordered_map<ObjGroupProxyType, HolderBasePtrType> objs_;
  /// Reverse lookup map from an object pointer to the proxy
  std::unordered_map<void*, ObjGroupProxyType> obj_to_proxy_;
  /// Messages that are pending creation for delivery
  std::unordered_map<ObjGroupProxyType, std::vector<ActionType>> pending_;
  /// Map of object groups' labels
  std::unordered_map<ObjGroupProxyType, std::string> labels_;
};

}} /* end namespace vt::objgroup */

#include "vt/objgroup/proxy/proxy_objgroup_elm.impl.h"
#include "vt/objgroup/proxy/proxy_objgroup.impl.h"
#include "vt/objgroup/manager.impl.h"

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_H*/
