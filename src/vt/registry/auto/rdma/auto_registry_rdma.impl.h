/*
//@HEADER
// *****************************************************************************
//
//                          auto_registry_rdma.impl.h
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

#if !defined INCLUDED_VT_REGISTRY_AUTO_RDMA_AUTO_REGISTRY_RDMA_IMPL_H
#define INCLUDED_VT_REGISTRY_AUTO_RDMA_AUTO_REGISTRY_RDMA_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/auto/rdma/auto_registry_rdma.h"

namespace vt { namespace auto_registry {

inline AutoActiveRDMAPutType getAutoHandlerRDMAPut(HandlerType const handler) {
  using ContainerType = AutoActiveRDMAPutContainerType;

  auto const han_id = HandlerManager::getHandlerIdentifier(handler);
  return getAutoRegistryGen<ContainerType>().at(han_id).getFun();
}

template <typename MsgT, ActiveTypedRDMAPutFnType<MsgT>* f>
inline HandlerType makeAutoHandlerRDMAPut() {
  using FunctorT = FunctorAdapter<ActiveTypedRDMAPutFnType<MsgT>, f>;
  using ContainerType = AutoActiveRDMAPutContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveRDMAPutType>;
  using FuncType = ActiveRDMAPutFnPtrType;

  auto const id =
    RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
  constexpr auto reg_type = RegistryTypeEnum::RegRDMAPut;
  return HandlerManager::makeHandler(false, false, id, reg_type);
}

inline AutoActiveRDMAGetType getAutoHandlerRDMAGet(HandlerType const handler) {
  using ContainerType = AutoActiveRDMAGetContainerType;

  auto const han_id = HandlerManager::getHandlerIdentifier(handler);
  return getAutoRegistryGen<ContainerType>().at(han_id).getFun();
}

template <typename MsgT, ActiveTypedRDMAGetFnType<MsgT>* f>
inline HandlerType makeAutoHandlerRDMAGet() {
  using FunctorT = FunctorAdapter<ActiveTypedRDMAGetFnType<MsgT>, f>;
  using ContainerType = AutoActiveRDMAGetContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveRDMAGetType>;
  using FuncType = ActiveRDMAGetFnPtrType;

  auto const id =
    RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
  constexpr auto reg_type = RegistryTypeEnum::RegRDMAGet;
  return HandlerManager::makeHandler(false, false, id, reg_type);
}

}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_VT_REGISTRY_AUTO_RDMA_AUTO_REGISTRY_RDMA_IMPL_H*/
