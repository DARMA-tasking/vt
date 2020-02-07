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
#include "vt/utils/static_checks/msg_ptr.h"
#include "vt/rdmahandle/handle.fwd.h"

namespace vt { namespace objgroup { namespace proxy {

template <typename ObjT>
struct Proxy {

  Proxy() = default;
  Proxy(Proxy const&) = default;
  Proxy(Proxy&&) = default;
  Proxy& operator=(Proxy const&) = default;

  explicit Proxy(ObjGroupProxyType in_proxy)
    : proxy_(in_proxy)
  { }

public:

  /*
   * Broadcast a msg to this object group with a handler
   */
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void broadcast(MsgT* msg) const;
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn>
  void broadcast(MsgSharedPtr<MsgT> msg) const;
  template <typename MsgT, ActiveObjType<MsgT, ObjT> fn, typename... Args>
  void broadcast(Args&&... args) const;

  /*
   * Reduce over the objgroup
   */

  template <
    typename OpT = collective::None,
    typename MsgPtrT,
    typename MsgT = typename util::MsgPtrType<MsgPtrT>::MsgType,
    ActiveTypedFnType<MsgT> *f = MsgT::template msgHandler<
      MsgT, OpT, collective::reduce::operators::ReduceCallback<MsgT>
    >
  >
  EpochType reduce(
    MsgPtrT msg, Callback<MsgT> cb, EpochType epoch = no_epoch,
    TagType tag = no_tag
  ) const;

  template <
    typename OpT = collective::None,
    typename FunctorT,
    typename MsgPtrT,
    typename MsgT = typename util::MsgPtrType<MsgPtrT>::MsgType,
    ActiveTypedFnType<MsgT> *f = MsgT::template msgHandler<MsgT, OpT, FunctorT>
  >
  EpochType reduce(
    MsgPtrT msg, EpochType epoch = no_epoch, TagType tag = no_tag
  ) const;

  template <
    typename MsgPtrT,
    typename MsgT = typename util::MsgPtrType<MsgPtrT>::MsgType,
    ActiveTypedFnType<MsgT> *f
  >
  EpochType reduce(
    MsgPtrT msg, EpochType epoch = no_epoch, TagType tag = no_tag
  ) const;

  /*
   * Get the local pointer to this object group residing in the current node
   * context
   */
  ObjT* get() const;

  /*
   * Get the underlying proxy bits that are used to identify the objgroup
   */
  ObjGroupProxyType getProxy() const;

  /*
   * Up- and down-cast the proxy---important due to type registration
   */
  template <typename BaseT>
  Proxy<BaseT> registerBaseCollective() const;
  template <typename BaseT>
  Proxy<BaseT> downcast() const;
  template <typename DerivedT>
  Proxy<DerivedT> upcast() const;

  /*
   * Destruct the objgroup collectively
   */
  void destroyCollective() const;

  /**
   * \brief Make a new RDMA handle for this objgroup---a collective invocation
   *
   * \param[in] size the local size for the handle
   * \param[in] is_uniform whether all handles have the same size
   *
   * \return the new RDMA handle
   */
  template <typename T>
  vt::rdma::Handle<T> makeHandleRDMA(std::size_t size, bool is_uniform) const;

  /**
   * \brief Destroy an RDMA handle created from this objgroup
   *
   * \param[in] handle the handle to destroy
   */
  template <typename T>
  void destroyHandleRDMA(vt::rdma::Handle<T> handle) const;

public:

  /*
   * Index the object group to get an element; can use operator[] or operator()
   */
  ProxyElm<ObjT> operator[](NodeType node) const;
  ProxyElm<ObjT> operator()(NodeType node) const;

private:
  ObjGroupProxyType proxy_ = no_obj_group;
};

}}} /* end namespace vt::objgroup::proxy */

#endif /*INCLUDED_VT_OBJGROUP_PROXY_PROXY_OBJGROUP_H*/
