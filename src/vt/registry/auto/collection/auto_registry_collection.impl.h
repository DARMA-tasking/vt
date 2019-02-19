/*
//@HEADER
// ************************************************************************
//
//                          auto_registry_collection.impl.h
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

#if !defined INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_IMPL_H
#define INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"

namespace vt { namespace auto_registry {

inline AutoActiveCollectionType getAutoHandlerCollection(
  HandlerType const& handler
) {
  using ContainerType = AutoActiveCollectionContainerType;
  return getAutoRegistryGen<ContainerType>().at(handler).getFun();
}

template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT>* f>
inline HandlerType makeAutoHandlerCollection(MsgT* const msg) {
  using FunctorT = FunctorAdapter<ActiveColTypedFnType<MsgT, ColT>, f>;
  using ContainerType = AutoActiveCollectionContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveCollectionType>;
  using FuncType = ActiveColFnPtrType;
  return RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
}

inline AutoActiveCollectionMemType getAutoHandlerCollectionMem(
  HandlerType const& handler
) {
  using ContainerType = AutoActiveCollectionMemContainerType;
  return getAutoRegistryGen<ContainerType>().at(handler).getFun();
}

template <
  typename ColT, typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f
>
inline HandlerType makeAutoHandlerCollectionMem(MsgT* const msg) {
  using FunctorT = FunctorAdapterMember<ActiveColMemberTypedFnType<MsgT, ColT>, f>;
  using ContainerType = AutoActiveCollectionMemContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveCollectionMemType>;
  using FuncType = ActiveColMemberFnPtrType;
  return RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
}

inline AutoActiveCollectionFetchType getAutoHandlerCollectionFetch(
  HandlerType handler
) {
  using ContainerType = AutoActiveCollectionFetchContainerType;
  return getAutoRegistryGen<ContainerType>().at(handler).getFun();
}

template <
  typename ColT, typename MsgT, typename FetchT,
  ActiveColFetchTypedFnType<MsgT, ColT, FetchT> f
>
inline HandlerType makeAutoHandlerCollectionFetch(MsgT* const msg) {
  using FunctorT = FunctorAdapterMember<ActiveColFetchTypedFnType<MsgT, ColT, FetchT>, f>;
  using ContainerType = AutoActiveCollectionFetchContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveCollectionFetchType>;
  using FuncType = ActiveColFetchFnPtrType;
  return RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
}


}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_IMPL_H*/
