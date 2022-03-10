/*
//@HEADER
// *****************************************************************************
//
//                       auto_registry_collection.impl.h
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

#if !defined INCLUDED_VT_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_IMPL_H
#define INCLUDED_VT_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"

namespace vt { namespace auto_registry {

inline AutoActiveCollectionType const&
getAutoHandlerCollection(HandlerType const handler) {
  using ContainerType = AutoActiveCollectionContainerType;

  auto const han_id = HandlerManagerType::getHandlerIdentifier(handler);
  return getAutoRegistryGen<ContainerType>().at(han_id).getFun();
}

template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT>* f>
inline HandlerType makeAutoHandlerCollection() {
  using FunctorT = FunctorAdapter<ActiveColTypedFnType<MsgT, ColT>, f, MsgT, ColT>;
  using ContainerType = AutoActiveCollectionContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveCollectionType>;
  using FuncType = ActiveColFnPtrType;

  auto const id =
    RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
  constexpr auto reg_type = RegistryTypeEnum::RegVrtCollection;
  auto handler = HandlerManager::makeHandler(false, false, id, reg_type);
  return handler;
}

inline AutoActiveCollectionMemType const&
getAutoHandlerCollectionMem(HandlerType const handler) {
  using ContainerType = AutoActiveCollectionMemContainerType;

  auto const han_id = HandlerManagerType::getHandlerIdentifier(handler);
  return getAutoRegistryGen<ContainerType>().at(han_id).getFun();
}

template <
  typename ColT, typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f
>
inline HandlerType makeAutoHandlerCollectionMem() {
  using FunctorT = FunctorAdapterMember<
    ActiveColMemberTypedFnType<MsgT, ColT>, f, ColT, MsgT
  >;
  using ContainerType = AutoActiveCollectionMemContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveCollectionMemType>;
  using FuncType = ActiveColMemberFnPtrType;

  auto const id =
    RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
  constexpr auto reg_type = RegistryTypeEnum::RegVrtCollectionMember;
  auto handler = HandlerManager::makeHandler(false, false, id, reg_type);
  return handler;
}

template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT>* f>
void setHandlerTraceNameColl(std::string const& name, std::string const& parent) {
#if vt_check_enabled(trace_enabled)
  auto const handler = makeAutoHandlerCollection<ColT,MsgT,f>();
  auto const trace_id = handlerTraceID(handler);
  trace::TraceRegistry::setTraceName(trace_id, name, parent);
#endif
}

template <typename ColT, typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f>
void setHandlerTraceNameCollMem(std::string const& name, std::string const& parent) {
#if vt_check_enabled(trace_enabled)
  auto const handler = makeAutoHandlerCollectionMem<ColT,MsgT,f>();
  auto const trace_id = handlerTraceID(handler);
  trace::TraceRegistry::setTraceName(trace_id, name, parent);
#endif
}

}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_VT_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_IMPL_H*/
