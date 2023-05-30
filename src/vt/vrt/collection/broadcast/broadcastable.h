/*
//@HEADER
// *****************************************************************************
//
//                               broadcastable.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H
#define INCLUDED_VT_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H

#include "vt/config.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/messaging/pending_send.h"
#include "vt/utils/fntraits/fntraits.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct Broadcastable : BaseProxyT {
  Broadcastable() = default;
  Broadcastable(Broadcastable const&) = default;
  Broadcastable(Broadcastable&&) = default;
  Broadcastable(VirtualProxyType const in_proxy);
  Broadcastable& operator=(Broadcastable const&) = default;

  template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
  [[deprecated("For simple create and broadcast message use broadcast(Args...),\
   otherwise use broadcastMsg(MsgPtrThief)")]]
  messaging::PendingSend broadcast(MsgT* msg) const;
  template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
  [[deprecated("For simple create and broadcast message use broadcast(Args...),\
   otherwise use broadcastMsg(MsgPtrThief)")]]
  messaging::PendingSend broadcast(MsgSharedPtr<MsgT> msg) const;

  /**
   * \brief Rooted broadcast with action function handler
   * \note Takes ownership of the supplied message
   *
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
  messaging::PendingSend broadcastMsg(messaging::MsgPtrThief<MsgT> msg) const;

  /**
   * \brief Rooted broadcast with action function handler
   *
   * \param[in] args arguments needed for creteating the message
   *
   * \return a pending send
   */
  template <
    typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f, typename... Args
  >
  messaging::PendingSend broadcast(Args&&... args) const;

  template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
  [[deprecated("For simple create and broadcast message use broadcast(Args...),\
   otherwise use broadcastMsg(MsgPtrThief)")]]
  messaging::PendingSend broadcast(MsgT* msg) const;
  template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
  [[deprecated("For simple create and broadcast message use broadcast(Args...),\
   otherwise use broadcastMsg(MsgPtrThief)")]]
  messaging::PendingSend broadcast(MsgSharedPtr<MsgT> msg) const;

  /**
   * \brief Rooted broadcast with action member handler
   * \note Takes ownership of the supplied message
   *
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
  messaging::PendingSend broadcastMsg(messaging::MsgPtrThief<MsgT> msg) const;

  /**
   * \brief Rooted broadcast with action member handler
   *
   * \param[in] args arguments needed for creteating the message
   *
   * \return a pending send
   */
  template <
    typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f, typename... Args
  >
  messaging::PendingSend broadcast(Args&&... args) const;

  /**
   * \brief Collective broadcast with action function handler
   * \note Takes ownership of the supplied message
   *
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
  messaging::PendingSend broadcastCollectiveMsg(messaging::MsgPtrThief<MsgT> msg) const;

  /**
   * \brief Create message (with action function handler) and broadcast it in a
   * collective manner to the collection
   *
   * \param[in] args arguments needed for creteating the message
   *
   * \return a pending send
   */
  template <
    typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f, typename... Args
  >
  messaging::PendingSend broadcastCollective(Args&&... args) const;

  /**
   * \brief Collective broadcast with action member handler
   * \note Takes ownership of the supplied message
   *
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
  messaging::PendingSend broadcastCollectiveMsg(messaging::MsgPtrThief<MsgT> msg) const;

  /**
   * \brief Create message (with action member handler) and broadcast it in a
   * collective manner to the collection
   *
   * \param[in] args arguments needed for creteating the message
   *
   * \return a pending send
   */
  template <
    typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f, typename... Args
  >
  messaging::PendingSend broadcastCollective(Args&&... args) const;

  /**
   * \brief Invoke member message handler on all collection elements
   * The function will be invoked inline without going through scheduler
   *
   * \param[in] args arguments for creating the message
   */
  template <
    typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f, typename... Args
  >
  void invokeCollective(Args&&... args) const;

  /**
   * \brief Invoke message handler on all collection elements
   * The function will be invoked inline without going through scheduler
   *
   * \param[in] args arguments for creating the message
   */
  template <
    typename MsgT, ActiveColTypedFnType<MsgT, typename MsgT::CollectionType>* f,
    typename... Args
  >
  void invokeCollective(Args&&... args) const;

  /**
   * \brief Rooted broadcast with action function handler
   * \note Takes ownership of the supplied message
   *
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <auto f>
  messaging::PendingSend broadcastMsg(
    messaging::MsgPtrThief<typename ObjFuncTraits<decltype(f)>::MsgT> msg
  ) const {
    using MsgT = typename ObjFuncTraits<decltype(f)>::MsgT;
    return broadcastMsg<MsgT, f>(msg);
  }

  /**
   * \brief Rooted broadcast with action function handler
   *
   * \param[in] args arguments needed for creteating the message
   *
   * \return a pending send
   */
  template <auto f, typename... Args>
  messaging::PendingSend broadcast(Args&&... args) const;

  /**
   * \brief Collective broadcast with action function handler
   * \note Takes ownership of the supplied message
   *
   * \param[in] msg the message
   *
   * \return a pending send
   */
  template <auto f>
  messaging::PendingSend broadcastCollectiveMsg(
    messaging::MsgPtrThief<typename ObjFuncTraits<decltype(f)>::MsgT> msg
  ) const {
    using MsgT = typename ObjFuncTraits<decltype(f)>::MsgT;
    return broadcastCollectiveMsg<MsgT, f>(msg);
  }

  /**
   * \brief Create message (with action function handler) and broadcast it in a
   * collective manner to the collection
   *
   * \param[in] args arguments needed for creteating the message
   *
   * \return a pending send
   */
  template <auto f, typename... Args>
  messaging::PendingSend broadcastCollective(Args&&... args) const;

  /**
   * \brief Invoke member message handler on all collection elements
   * The function will be invoked inline without going through scheduler
   *
   * \param[in] args arguments for creating the message
   */
  template <auto f, typename... Args>
  void invokeCollective(Args&&... args) const;

};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H*/
