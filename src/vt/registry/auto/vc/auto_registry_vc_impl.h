/*
//@HEADER
// ************************************************************************
//
//                          auto_registry_vc_impl.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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
#if !defined INCLUDED_REGISTRY_AUTO_VC_AUTO_REGISTRY_VC_IMPL_H
#define INCLUDED_REGISTRY_AUTO_VC_AUTO_REGISTRY_VC_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry.h"
#include "vt/vrt/context/context_vrt_funcs.h"

#include <vector>

namespace vt { namespace auto_registry {

using namespace vrt;

template <typename VrtT, typename MsgT, ActiveVrtTypedFnType<MsgT, VrtT>* f>
inline HandlerType makeAutoHandlerVC(MsgT* const __attribute__((unused)) msg) {
  using FunctorT = FunctorAdapter<ActiveVrtTypedFnType<MsgT, VrtT>, f>;
  using ContainerType = AutoActiveVCContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveVCType>;
  using FuncType = ActiveVirtualFnPtrType;
  return RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
}

inline AutoActiveVCType getAutoHandlerVC(HandlerType const& handler) {
  using ContainerType = AutoActiveVCContainerType;
  return getAutoRegistryGen<ContainerType>().at(handler).getFun();
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_VC_AUTO_REGISTRY_VC_IMPL_H*/
