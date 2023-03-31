/*
//@HEADER
// *****************************************************************************
//
//                             proxy_objgroup_elm.h
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

#if !defined INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_ELM_H
#define INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_ELM_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_bits.h"
#include "vt/objgroup/active_func/active_func.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/pending_send.fwd.h"
#include "vt/utils/fntraits/fntraits.h"

namespace vt { namespace objgroup { namespace proxy {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct ObjGroupReconstructTagType { } ObjGroupReconstructTag { };
#pragma GCC diagnostic pop

/**
 * \struct ProxyElm
 *
 * \brief A indexed proxy to a object instance on a particular node
 *
 * A \c Proxy<ObjT> can be indexed to select a particular node, returning this
 * class. Once indexed, one can send a message to the particular node that has
 * been selected.
 */
template <typename ObjT>
struct ProxyElm {

  using PendingSendType = messaging::PendingSend;

  ProxyElm() = default;
  ProxyElm(ProxyElm const&) = default;
  ProxyElm(ProxyElm&&) = default;
  ProxyElm& operator=(ProxyElm const&) = default;

  /**
   * \internal \brief Create a new element proxy, called by the system
   *
   * \param[in] in_proxy the proxy ID
   * \param[in] in_node the node selected
   */
  ProxyElm(ObjGroupProxyType in_proxy, NodeType in_node)
    : proxy_(in_proxy), node_(in_node)
  { }

  /**
   * \brief Send a message to the node/element indexed by this proxy to be
   * delivered to the local object instance
   *
   * \param[in] msg raw pointer to the message
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  [[deprecated("For simple create and send message use send(Args...),\
   otherwise use sendMsg(MsgPtrThief)")]]
  void send(MsgT* msg) const;

  /**
   * \brief Send a message to the node/element indexed by this proxy to be
   * delivered to the local object instance
   *
   * \param[in] msg managed pointer to the message
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  [[deprecated("For simple create and send message use send(Args...),\
   otherwise use sendMsg(MsgPtrThief)")]]
  void send(MsgSharedPtr<MsgT> msg) const;

  /**
   * \brief Send a message to the node/element indexed by this proxy to be
   * delivered to the local object instance
   * \note Takes ownership of the supplied message
   *
   * \param[in] msg the message
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  PendingSendType sendMsg(messaging::MsgPtrThief<MsgT> msg) const;

  /**
   * \brief Send a message to the node/element indexed by this proxy to be
   * delivered to the local object instance
   * \note Takes ownership of the supplied message
   *
   * \param[in] msg the message
   */
  template <auto fn>
  decltype(auto) sendMsg(
    messaging::MsgPtrThief<typename ObjFuncTraits<decltype(fn)>::MsgT> msg
  ) const {
    using MsgT = typename ObjFuncTraits<decltype(fn)>::MsgT;
    return sendMsg<MsgT, fn>(msg);
  }

  /**
   * \brief Send a message to the node/element indexed by this proxy to be
   * delivered to the local object instance
   *
   * \param[in] args args to pass to the message constructor
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn, typename... Args>
  PendingSendType send(Args&&... args) const;

  /**
   * \brief Send a message to the node/element indexed by this proxy to be
   * delivered to the local object instance
   *
   * \param[in] args args to pass to the message constructor
   */
  template <auto fn, typename... Args>
  PendingSendType send(Args&&... args) const;

  /**
   * \brief Invoke locally a message handler on the node/element indexed by this proxy.
   * The message handler will be invoked inline without going through scheduler
   *
   * \param[in] args args to pass to the message constructor
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn, typename... Args>
  decltype(auto) invoke(Args&&... args) const;

  /**
   * \brief Invoke locally a function 'f' on the node/element indexed by this proxy.
   * The function will be invoked inline without going through scheduler
   *
   * \param[in] args function arguments
   */
  template <auto f, typename... Args>
  decltype(auto) invoke(Args&&... args) const;

  /**
   * \brief Update the local object instance pointer on this node. Must be run
   * on local node.
   *
   * \param[in] ObjGroupReconstructTagType tag to select overload
   * \param[in] args args to pass the object constructor
   */
  template <typename... Args>
  void update(ObjGroupReconstructTagType, Args&&... args) const;

  /**
   * \brief Get raw pointer to the local object instance residing on the
   * selected node.
   *
   * \warning If this is called on a non-local node it will return nullptr since
   * that object instance is not accessible.
   *
   * \warning Do not hold this raw pointer longer than the object group
   * lifetime. Once the object group is destroyed the pointer will no longer be
   * valid.
   *
   * \return raw pointer to the object
   */
  ObjT* get() const;

  /**
   * \brief Get the underlying proxy bits that are used to identify the objgroup
   *
   * \return the proxy ID
   */
  ObjGroupProxyType getProxy() const { return proxy_; }

  /**
   * \brief Get the node this element proxy is holding
   *
   * \return the node indexed
   */
  NodeType getNode() const { return node_; }

public:
  /**
   * \brief Serialize the element proxy
   *
   * \param[in] s serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  ObjGroupProxyType proxy_ = no_obj_group;              /**< The raw proxy ID bits */
  NodeType node_           = uninitialized_destination; /**< The indexed node */
};

template <>
struct ProxyElm<void> {
  explicit ProxyElm(NodeType in_node);

  /**
   * \brief Send a message to the node indexed by this proxy to be
   * delivered to the local object instance
   *
   * \param[in] args args to pass to the message constructor
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename... Args>
  void send(Args&&... args) const;

  /**
   * \brief Send a message to the node indexed by this proxy to be
   * delivered to the local object instance
   *
   * \param[in] args args to pass to the message constructor
   */
  template <auto f, typename... Args>
  void send(Args&&... args) const {
    using MsgT = typename FuncTraits<decltype(f)>::MsgT;
    send<MsgT, f>(std::forward<Args>(args)...);
  }

private:
  NodeType node_ = uninitialized_destination; /**< The indexed node */
};

using DefaultProxyElm = ProxyElm<void>;

}}} /* end namespace vt::objgroup::proxy */

#endif /*INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_ELM_H*/
