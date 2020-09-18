/*
//@HEADER
// *****************************************************************************
//
//                               broadcastable.h
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

#if !defined INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H
#define INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H

#include "vt/config.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/messaging/pending_send.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct Broadcastable : BaseProxyT {
  Broadcastable() = default;
  Broadcastable(Broadcastable const&) = default;
  Broadcastable(Broadcastable&&) = default;
  Broadcastable(VirtualProxyType const in_proxy);
  Broadcastable& operator=(Broadcastable const&) = default;

  template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
  messaging::PendingSend broadcast(MsgT* msg) const;
  template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
  messaging::PendingSend broadcast(MsgSharedPtr<MsgT> msg) const;
  template <
    typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f, typename... Args
  >
  messaging::PendingSend broadcast(Args&&... args) const;

  template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
  messaging::PendingSend broadcast(MsgT* msg) const;
  template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
  messaging::PendingSend broadcast(MsgSharedPtr<MsgT> msg) const;
  template <
    typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f, typename... Args
  >
  messaging::PendingSend broadcast(Args&&... args) const;

  template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
  messaging::PendingSend broadcastCollective(MsgT* msg) const;
  template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
  messaging::PendingSend broadcastCollective(MsgSharedPtr<MsgT> msg) const;
  template <
    typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f, typename... Args
  >
  messaging::PendingSend broadcastCollective(Args&&... args) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H*/
