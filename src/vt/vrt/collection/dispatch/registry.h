/*
//@HEADER
// *****************************************************************************
//
//                                  registry.h
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

#if !defined INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_H
#define INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/vrt/collection/dispatch/dispatch.h"

#include <vector>
#include <memory>

namespace vt { namespace vrt { namespace collection {

using AutoHandlerType           = auto_registry::AutoHandlerType;
using DispatchBaseType          = DispatchCollectionBase;
using DispatchBasePtrType       = DispatchCollectionBase*;
using DispatchBaseOwningPtrType = std::unique_ptr<DispatchCollectionBase>;
using RegistryTLType            = std::vector<DispatchBaseOwningPtrType>;

inline RegistryTLType& getTLRegistry();

template <typename MsgT, typename ColT>
struct RegistrarVrt {
  RegistrarVrt();
  AutoHandlerType index;
};

template <typename MsgT, typename ColT>
struct RegistrarWrapperVrt {
  RegistrarVrt<MsgT,ColT> registrar;
};

template <typename MsgT, typename ColT>
struct VrtDispatchHolder {
  static AutoHandlerType const idx;
};

template <typename MsgT, typename ColT>
inline AutoHandlerType registerVrtDispatch();

inline DispatchBasePtrType getDispatch(AutoHandlerType const& han);

template <typename MsgT, typename ColT>
inline AutoHandlerType makeVrtDispatch(
  VirtualProxyType const& default_proxy = no_vrt_proxy
);

}}} /* end namespace vt::vrt::collection: */

#endif /*INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_H*/
