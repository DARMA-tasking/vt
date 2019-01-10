/*
//@HEADER
// ************************************************************************
//
//                          registry.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_IMPL_H
#define INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/dispatch/dispatch.h"
#include "vt/vrt/collection/dispatch/registry.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/registry/auto/auto_registry_common.h"

namespace vt { namespace vrt { namespace collection {

inline RegistryTLType& getTLRegistry() {
  #pragma sst keep
  static RegistryTLType reg;
  return reg;
}

template <typename MsgT, typename ColT>
RegistrarVrt<MsgT,ColT>::RegistrarVrt() {
  auto& reg = getTLRegistry();
  index = reg.size();
  reg.emplace_back(std::make_unique<DispatchCollection<ColT,MsgT>>());
}

template <typename MsgT, typename ColT>
inline AutoHandlerType registerVrtDispatch() {
  return RegistrarWrapperVrt<MsgT,ColT>().registrar.index;
}

template <typename MsgT, typename ColT>
AutoHandlerType const VrtDispatchHolder<MsgT,ColT>::idx =
  registerVrtDispatch<MsgT,ColT>();

inline DispatchBasePtrType getDispatch(AutoHandlerType const& han) {
  return getTLRegistry().at(han).get();
}

template <typename MsgT, typename ColT>
inline AutoHandlerType makeVrtDispatch(VirtualProxyType const& default_proxy) {
  auto const idx = VrtDispatchHolder<MsgT,ColT>::idx;
  if (default_proxy != no_vrt_proxy) {
    getDispatch(idx)->setDefaultProxy(default_proxy);
  }
  return idx;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_IMPL_H*/
