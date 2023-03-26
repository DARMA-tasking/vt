/*
//@HEADER
// *****************************************************************************
//
//                               sendable.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_SEND_SENDABLE_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_SEND_SENDABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/send/sendable.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/messages/param_col_msg.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
Sendable<ColT,IndexT,BaseProxyT>::Sendable(
  typename BaseProxyT::ProxyType const& in_proxy,
  typename BaseProxyT::ElementProxyType const& in_elm
) : BaseProxyT(in_proxy, in_elm)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename SerializerT>
void Sendable<ColT,IndexT,BaseProxyT>::serialize(SerializerT& s) {
  BaseProxyT::serialize(s);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
messaging::PendingSend Sendable<ColT,IndexT,BaseProxyT>::send(MsgT* msg) const {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy,elm_proxy);
  return theCollection()->sendMsg<MsgT, f>(proxy, msg);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColTypedFnType<MsgT,ColT> *f>
messaging::PendingSend Sendable<ColT,IndexT,BaseProxyT>::send(MsgSharedPtr<MsgT> msg) const {
  return send<MsgT,f>(msg.get());
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColTypedFnType<MsgT, ColT>* f>
messaging::PendingSend Sendable<ColT, IndexT, BaseProxyT>::sendMsg(
  messaging::MsgPtrThief<MsgT> msg
) const {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy, elm_proxy);
  return theCollection()->sendMsg<MsgT, f>(proxy, msg.msg_.get());
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColTypedFnType<MsgT,ColT> *f, typename... Args>
messaging::PendingSend Sendable<ColT,IndexT,BaseProxyT>::send(Args&&... args) const {
  return sendMsg<MsgT,f>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
messaging::PendingSend Sendable<ColT,IndexT,BaseProxyT>::send(MsgT* msg) const {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy,elm_proxy);
  return theCollection()->sendMsg<MsgT, f>(proxy, msg);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColMemberTypedFnType<MsgT,ColT> f>
messaging::PendingSend Sendable<ColT,IndexT,BaseProxyT>::send(MsgSharedPtr<MsgT> msg) const {
  return send<MsgT,f>(msg.get());
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
messaging::PendingSend Sendable<ColT, IndexT, BaseProxyT>::sendMsg(
  messaging::MsgPtrThief<MsgT> msg
) const {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy, elm_proxy);
  return theCollection()->sendMsg<MsgT, f>(proxy, msg.msg_.get());
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT, ActiveColMemberTypedFnType<MsgT,ColT> f, typename... Args
>
messaging::PendingSend Sendable<ColT,IndexT,BaseProxyT>::send(Args&&... args) const {
  return sendMsg<MsgT,f>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <auto f, typename... Params>
messaging::PendingSend Sendable<ColT,IndexT,BaseProxyT>::send(Params&&... params) const {
  using FnTrait = ObjFuncTraits<decltype(f)>;

  using MsgT = typename FnTrait::MsgT;
  if constexpr (std::is_same_v<MsgT, NoMsg>) {
    using Tuple = typename FnTrait::TupleType;
    using SendMsgT = ParamColMsg<Tuple, ColT>;
    auto msg = vt::makeMessage<SendMsgT>(std::forward<Params>(params)...);
    auto han = auto_registry::makeAutoHandlerCollectionMemParam<
      ColT, decltype(f), f, SendMsgT
    >();
    auto col_proxy = this->getCollectionProxy();
    auto elm_proxy = this->getElementProxy();
    auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy, elm_proxy);
    return theCollection()->sendMsgUntypedHandler<SendMsgT, ColT, IndexT>(
      proxy, msg.get(), han
    );
  } else {
    auto msg = makeMessage<MsgT>(std::forward<Params>(params)...);
    return sendMsg<MsgT, f>(msg);
  }

  // Silence nvcc warning (no longer needed for CUDA 11.7 and up)
  return messaging::PendingSend{std::nullptr_t{}};
}


}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_SEND_SENDABLE_IMPL_H*/
