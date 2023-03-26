/*
//@HEADER
// *****************************************************************************
//
//                             broadcastable.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BROADCAST_BROADCASTABLE_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_BROADCAST_BROADCASTABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/broadcast/broadcastable.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
Broadcastable<ColT,IndexT,BaseProxyT>::Broadcastable(
  VirtualProxyType const in_proxy
) : BaseProxyT(in_proxy)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
messaging::PendingSend Broadcastable<ColT,IndexT,BaseProxyT>::broadcast(MsgT* msg) const {
  auto proxy = this->getProxy();
  return theCollection()->broadcastMsg<MsgT, ColT, f>(proxy,msg);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
messaging::PendingSend Broadcastable<ColT,IndexT,BaseProxyT>::broadcast(
  MsgSharedPtr<MsgT> msg
) const {
  return broadcast<MsgT,f>(msg.get());
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f, typename... Args
>
messaging::PendingSend Broadcastable<ColT,IndexT,BaseProxyT>::broadcast(Args&&... args) const {
  return broadcastMsg<MsgT, f>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
messaging::PendingSend
Broadcastable<ColT, IndexT, BaseProxyT>::broadcastMsg(messaging::MsgPtrThief<MsgT> msg) const {
  auto proxy = this->getProxy();

  return theCollection()->broadcastMsg<MsgT, ColT, f>(proxy, msg.msg_.get());
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
messaging::PendingSend Broadcastable<ColT,IndexT,BaseProxyT>::broadcast(
  MsgSharedPtr<MsgT> msg
) const {
  return broadcast<MsgT,f>(msg.get());
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f, typename... Args
>
messaging::PendingSend Broadcastable<ColT,IndexT,BaseProxyT>::broadcast(Args&&... args) const {
  return broadcastMsg<MsgT,f>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
messaging::PendingSend
Broadcastable<ColT, IndexT, BaseProxyT>::broadcastMsg(messaging::MsgPtrThief<MsgT> msg) const {
  auto proxy = this->getProxy();
  return theCollection()->broadcastMsg<MsgT, ColT, f>(proxy, msg.msg_.get());
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
messaging::PendingSend Broadcastable<ColT,IndexT,BaseProxyT>::broadcast(MsgT* msg) const {
  auto proxy = this->getProxy();
  return theCollection()->broadcastMsg<MsgT, ColT, f>(proxy, msg);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
messaging::PendingSend
Broadcastable<ColT, IndexT, BaseProxyT>::broadcastCollectiveMsg(messaging::MsgPtrThief<MsgT> msg) const {
  auto proxy = this->getProxy();
  return theCollection()->broadcastCollectiveMsg<MsgT, f>(proxy, msg);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f, typename... Args
>
messaging::PendingSend
Broadcastable<ColT, IndexT, BaseProxyT>::broadcastCollective(Args&&... args) const {
  return broadcastCollectiveMsg<MsgT, f>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
messaging::PendingSend
Broadcastable<ColT, IndexT, BaseProxyT>::broadcastCollectiveMsg(messaging::MsgPtrThief<MsgT> msg) const {
  auto proxy = this->getProxy();
  return theCollection()->broadcastCollectiveMsg<MsgT, f>(proxy, msg);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f, typename... Args
>
messaging::PendingSend
Broadcastable<ColT, IndexT, BaseProxyT>::broadcastCollective(Args&&... args) const {
  return broadcastCollectiveMsg<MsgT, f>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f, typename... Args
>
void
Broadcastable<ColT, IndexT, BaseProxyT>::invokeCollective(Args&&... args) const {
  auto proxy = this->getProxy();
  auto msg = makeMessage<MsgT>(std::forward<Args>(args)...);
  return theCollection()->invokeCollectiveMsg<MsgT, f>(proxy, msg);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT, ActiveColTypedFnType<MsgT, typename MsgT::CollectionType>* f,
  typename... Args
>
void
Broadcastable<ColT, IndexT, BaseProxyT>::invokeCollective(Args&&... args) const {
  auto proxy = this->getProxy();
  auto msg = makeMessage<MsgT>(std::forward<Args>(args)...);
  return theCollection()->invokeCollectiveMsg<MsgT, f>(proxy, msg);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <auto f, typename... Params>
messaging::PendingSend
Broadcastable<ColT, IndexT, BaseProxyT>::broadcast(Params&&... params) const {
  using MsgT = typename ObjFuncTraits<decltype(f)>::MsgT;
  if constexpr (std::is_same_v<MsgT, NoMsg>) {
    using Tuple = typename ObjFuncTraits<decltype(f)>::TupleType;
    using SendMsgT = ParamColMsg<Tuple, ColT>;
    auto msg = vt::makeMessage<SendMsgT>(std::forward<Params>(params)...);
    auto han = auto_registry::makeAutoHandlerCollectionMemParam<
      ColT, decltype(f), f, SendMsgT
    >();
    auto proxy = this->getProxy();
    return theCollection()->broadcastMsgUntypedHandler<SendMsgT, ColT, IndexT>(
      proxy, msg.get(), han, true
    );
  } else {
    auto msg = makeMessage<MsgT>(std::forward<Params>(params)...);
    return broadcastMsg<MsgT, f>(msg);
  }

  // Silence nvcc warning (no longer needed for CUDA 11.7 and up)
  return messaging::PendingSend{std::nullptr_t{}};
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <auto f, typename... Params>
messaging::PendingSend
Broadcastable<ColT, IndexT, BaseProxyT>::broadcastCollective(Params&&... params) const {
  using MsgT = typename ObjFuncTraits<decltype(f)>::MsgT;
  if constexpr (std::is_same_v<MsgT, NoMsg>) {
    using Tuple = typename ObjFuncTraits<decltype(f)>::TupleType;
    using SendMsgT = ParamColMsg<Tuple, ColT>;
    auto msg = vt::makeMessage<SendMsgT>(std::forward<Params>(params)...);
    auto han = auto_registry::makeAutoHandlerCollectionMemParam<
      ColT, decltype(f), f, SendMsgT
    >();
    auto proxy = this->getProxy();
    msg->setVrtHandler(han);
    return theCollection()->broadcastCollectiveMsgImpl<SendMsgT, ColT>(
      proxy, msg, true
    );
  } else {
    auto msg = makeMessage<MsgT>(std::forward<Params>(params)...);
    return broadcastCollectiveMsg<MsgT, f>(msg);
  }

  // Silence nvcc warning (no longer needed for CUDA 11.7 and up)
  return messaging::PendingSend{std::nullptr_t{}};
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <auto f, typename... Params>
void
Broadcastable<ColT, IndexT, BaseProxyT>::invokeCollective(Params&&... params) const {
  using MsgT = typename ObjFuncTraits<decltype(f)>::MsgT;
    auto proxy = this->getProxy();
  if constexpr (std::is_same_v<MsgT, NoMsg>) {
    theCollection()->invokeCollective<ColT, f>(
      proxy, std::forward<Params>(params)...
    );
  } else {
    auto msg = makeMessage<MsgT>(std::forward<Params>(params)...);
    return theCollection()->invokeCollectiveMsg<MsgT, f>(proxy, msg);
  }
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_BROADCAST_BROADCASTABLE_IMPL_H*/
