/*
//@HEADER
// *****************************************************************************
//
//                           auto_registry_vc_impl.h
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
#if !defined INCLUDED_VT_REGISTRY_AUTO_VC_AUTO_REGISTRY_VC_IMPL_H
#define INCLUDED_VT_REGISTRY_AUTO_VC_AUTO_REGISTRY_VC_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry.h"
#include "vt/vrt/context/context_vrt_funcs.h"

#include <vector>

namespace vt { namespace auto_registry {

using namespace vrt;

template <typename VrtT, typename MsgT, ActiveVrtTypedFnType<MsgT, VrtT>* f>
inline HandlerType makeAutoHandlerVC() {
  using FunctorT = FunctorAdapter<ActiveVrtTypedFnType<MsgT, VrtT>, f, MsgT, VrtT>;
  using ContainerType = AutoActiveVCContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveVCType>;
  using FuncType = ActiveVirtualFnPtrType;

  auto const id =
    RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
  constexpr auto reg_type = RegistryTypeEnum::RegVrt;
  return HandlerManager::makeHandler(false, false, id, reg_type);
}

inline AutoActiveVCType const& getAutoHandlerVC(HandlerType const handler) {
  using ContainerType = AutoActiveVCContainerType;

  auto const han_id = HandlerManager::getHandlerIdentifier(handler);
  return getAutoRegistryGen<ContainerType>().at(han_id).getFun();
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_VT_REGISTRY_AUTO_VC_AUTO_REGISTRY_VC_IMPL_H*/
