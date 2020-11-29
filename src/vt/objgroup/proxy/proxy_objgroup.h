/*
//@HEADER
// *****************************************************************************
//
//                               proxy_objgroup.h
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

#if !defined INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_H
#define INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_bits.h"
#include "vt/objgroup/proxy/proxy_objgroup_elm.h"
#include "vt/objgroup/active_func/active_func.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/pipe/pipe_callback_only.h"
#include "vt/collective/reduce/operators/functors/none_op.h"
#include "vt/collective/reduce/operators/callback_op.h"
#include "vt/collective/reduce/reduce_scope.h"
#include "vt/utils/static_checks/msg_ptr.h"
#include "vt/rdmahandle/handle.fwd.h"
#include "vt/rdmahandle/handle_set.fwd.h"
#include "vt/messaging/pending_send.h"

namespace vt { namespace objgroup { namespace proxy {

/**
 * \struct Proxy
 *
 * \brief A indexable proxy to object instances on all nodes that are tied
 * together with a common ID
 *
 * After creating an objgroup, a Proxy<ObjT> is returned which can be used to
 * perform distributed operations on that object instance across all nodes.
 *
 * Proxies are very inexpensive to copy and move around the system.
 */
template <typename ObjT>
struct Proxy {
  using ReduceStamp = collective::reduce::ReduceStamp;

  using PendingSendType = messaging::PendingSend;

  Proxy() = default;
  Proxy(Proxy const&) = default;
  Proxy(Proxy&&) = default;
  Proxy& operator=(Proxy const&) = default;

  /**
   * \internal \brief Create a new proxy, called by the system.
   *
   * \param[in] in_proxy the proxy ID
   */
  explicit Proxy(ObjGroupProxyType in_proxy)
    : proxy_(in_proxy)
  { }

public:

  /**
   * \brief Broadcast a message to all nodes to be delivered to the local object
   * instance
   *
   * \param[in] msg raw pointer to the message
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  [[deprecated("For simple create and broadcast message use broadcast(Args...),\
   otherwise use broadcastMsg(MsgPtrThief)")]]
  void broadcast(MsgT* msg) const;

  /**
   * \brief Broadcast a message to all nodes to be delivered to the local object
   * instance
   *
   * \param[in] msg managed pointer to the message
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  [[deprecated("For simple create and broadcast message use broadcast(Args...),\
   otherwise use broadcastMsg(MsgPtrThief)")]]
  void broadcast(MsgPtr<MsgT> msg) const;

  /**
   * \brief Broadcast a message to all nodes to be delivered to the local object
   * instance
   * \note Takes ownership of the supplied message
   *
   * \param[in] msg the message
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void broadcastMsg(messaging::MsgPtrThief<MsgT> msg) const;

  /**
   * \brief Broadcast a message to all nodes to be delivered to the local object
   * instance
   *
   * \param[in] args args to pass to the message constructor
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn, typename... Args>
  void broadcast(Args&&... args) const;

  /**
   * \brief Reduce over the objgroup instances on each node with a callback
   * target.
   *
   * \param[in] msg the reduction message
   * \param[in] cb the callback to trigger after the reduction is finished
   * \param[in] stamp the stamp to identify the reduction
   *
   * \return the PendingSend associated with the reduce
   */
  template <
    typename OpT = collective::None,
    typename MsgPtrT,
    typename MsgT = typename util::MsgPtrType<MsgPtrT>::MsgType,
    ActiveTypedFnType<MsgT> *f = MsgT::template msgHandler<
      MsgT, OpT, collective::reduce::operators::ReduceCallback<MsgT>
    >
  >
  PendingSendType reduce(
    MsgPtrT msg, Callback<MsgT> cb, ReduceStamp stamp = ReduceStamp{}
  ) const;

  /**
   * \brief Reduce over the objgroup instances on each node with a functor
   * target.
   *
   * \param[in] msg the reduction message
   * \param[in] stamp the stamp to identify the reduction
   *
   * \return the PendingSend associated with the reduce
   */
  template <
    typename OpT = collective::None,
    typename FunctorT,
    typename MsgPtrT,
    typename MsgT = typename util::MsgPtrType<MsgPtrT>::MsgType,
    ActiveTypedFnType<MsgT> *f = MsgT::template msgHandler<MsgT, OpT, FunctorT>
  >
  PendingSendType reduce(MsgPtrT msg, ReduceStamp stamp = ReduceStamp{}) const;

  /**
   * \brief Reduce over the objgroup instance on each node with target specified
   * in reduction message type.
   *
   * \param[in] msg the reduction message
   * \param[in] stamp the stamp to identify the reduction
   *
   * \return the PendingSend associated with the reduce
   */
  template <
    typename MsgPtrT,
    typename MsgT = typename util::MsgPtrType<MsgPtrT>::MsgType,
    ActiveTypedFnType<MsgT> *f
  >
  PendingSendType reduce(MsgPtrT msg, ReduceStamp stamp = ReduceStamp{}) const;

  /**
   * \brief Get raw pointer to the local object instance residing on the current
   * node.
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
  ObjGroupProxyType getProxy() const;

  /**
   * \brief Register the base class type for this objgroup, returning a valid
   * proxy to operate on the base.
   *
   * \return base class proxy
   */
  template <typename BaseT>
  Proxy<BaseT> registerBaseCollective() const;

  /**
   * \brief Downcast the proxy
   *
   * \warning In most cases this should not be called by users. It can have
   * dangerous behavior depending on how its used. Use \c registerBaseCollective
   * to get a proxy to the base.
   *
   * \return the typed proxy to the base
   */
  template <typename BaseT>
  Proxy<BaseT> downcast() const;

  /**
   * \brief Upcast the proxy
   *
   * \warning In most cases this should not be called by users. It can have
   * dangerous behavior depending on how its used. Use \c registerBaseCollective
   * to get a proxy to the base.
   *
   * \return the typed proxy to the derived
   */
  template <typename DerivedT>
  Proxy<DerivedT> upcast() const;

  /**
   * \brief Collective destroy this objgroup instance on all nodes
   */
  void destroyCollective() const;

  /**
   * \brief Make a new RDMA handle for this objgroup---a collective invocation
   *
   * \param[in] count the local count of T for this handle
   * \param[in] is_uniform whether all handles have the same count
   *
   * \return the new RDMA handle
   */
  template <typename T>
  vt::rdma::Handle<T> makeHandleRDMA(std::size_t count, bool is_uniform) const;

  /**
   * \brief Destroy an RDMA handle created from this objgroup
   *
   * \param[in] handle the handle to destroy
   */
  template <typename T>
  void destroyHandleRDMA(vt::rdma::Handle<T> handle) const;

  /**
   * \brief Make a new set of RDMA handles for this objgroup---a collective
   * invocation. This is the overload for a potentially sparse set of handles
   * with a non-zero starting index.
   *
   * \param[in] max_elm the largest lookup key on any node
   * \param[in] map a map of the handles and corresponding counts to create
   * \param[in] is_uniform whether all handles have the same count
   *
   * \return the new RDMA handle
   */
  template <typename T>
  vt::rdma::HandleSet<T> makeHandleSetRDMA(
    int32_t max_elm,
    std::unordered_map<int32_t, std::size_t> const& map,
    bool is_uniform
  ) const;

  /**
   * \brief Make a new set of RDMA handles for this objgroup---a collective
   * invocation. This is the overload for a dense, start at zero set of handles
   * to create.
   *
   * \param[in] max_elm the largest lookup key on any node
   * \param[in] vec a vector of the handle counts to create
   * \param[in] is_uniform whether all handles have the same count
   *
   * \return the new RDMA handle
   */
  template <typename T>
  vt::rdma::HandleSet<T> makeHandleSetRDMA(
    int32_t max_elm,
    std::vector<std::size_t> const& vec,
    bool is_uniform
  ) const;

  /**
   * \brief Destroy a set of RDMA handles created from this objgroup
   *
   * \param[in] handle the handle set to destroy
   */
  template <typename T>
  void destroyHandleSetRDMA(vt::rdma::HandleSet<T> handle) const;

public:

  /**
   * \brief Index the proxy to get the element proxy for a particular node
   *
   * \param[in] node the desired node
   *
   * \return an indexed proxy to that node
   */
  ProxyElm<ObjT> operator[](NodeType node) const;

  /**
   * \brief Index the proxy to get the element proxy for a particular node
   *
   * \param[in] node the desired node
   *
   * \return an indexed proxy to that node
   */
  ProxyElm<ObjT> operator()(NodeType node) const;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | proxy_;
  }

private:
  ObjGroupProxyType proxy_ = no_obj_group; /**< The raw proxy ID bits */
};

}}} /* end namespace vt::objgroup::proxy */

#endif /*INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_H*/
