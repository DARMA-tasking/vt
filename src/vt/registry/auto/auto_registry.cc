/*
//@HEADER
// *****************************************************************************
//
//                               auto_registry.cc
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

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/registry/registry.h"

#include <cassert>

namespace vt { namespace auto_registry {

#if vt_check_enabled(trace_enabled)

template <typename ContType>
trace::TraceEntryIDType getTraceID(HandlerType const handler) {
  auto const han_id = HandlerManagerType::getHandlerIdentifier(handler);
  return getAutoRegistryGen<ContType>().at(han_id).theTraceID();
}

trace::TraceEntryIDType
handlerTraceID(HandlerType const handler) {
  auto const reg_type = HandlerManager::getHandlerRegistryType(handler);
  switch (reg_type) {
  case RegistryTypeEnum::RegGeneral: {
    if (HandlerManagerType::isHandlerFunctor(handler)) {
      return getTraceID<AutoActiveFunctorContainerType>(handler);
    }

    return getTraceID<AutoActiveContainerType>(handler);
  }

  case RegistryTypeEnum::RegMap: {
    if (HandlerManagerType::isHandlerFunctor(handler)) {
      return getTraceID<AutoActiveMapFunctorContainerType>(handler);
    }

    return getTraceID<AutoActiveMapContainerType>(handler);
  }

  case RegistryTypeEnum::RegVrt: {
    return getTraceID<AutoActiveVCContainerType>(handler);
  }

  case RegistryTypeEnum::RegObjGroup: {
    return getTraceID<AutoActiveObjGroupContainerType>(handler);
  }

  case RegistryTypeEnum::RegVrtCollection: {
    return getTraceID<AutoActiveCollectionContainerType>(handler);
  }

  case RegistryTypeEnum::RegVrtCollectionMember: {
    return getTraceID<AutoActiveCollectionMemContainerType>(handler);
  }

  case RegistryTypeEnum::RegRDMAGet: {
    return getTraceID<AutoActiveRDMAGetContainerType>(handler);
  }

  case RegistryTypeEnum::RegRDMAPut: {
    return getTraceID<AutoActiveRDMAPutContainerType>(handler);
  }

  case RegistryTypeEnum::RegSeed: {
    return getTraceID<AutoActiveSeedMapContainerType>(handler);
  }

  default: {
    assert(0 && "Should not be reachable");
    return trace::TraceEntryIDType{};
  }
  }
}

#endif

}} // end namespace vt::auto_registry
