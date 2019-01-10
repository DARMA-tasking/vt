/*
//@HEADER
// ************************************************************************
//
//                          auto_registry_map.h
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

#if !defined INCLUDED_REGISTRY_AUTO_MAP_AUTO_REGISTRY_MAP_H
#define INCLUDED_REGISTRY_AUTO_MAP_AUTO_REGISTRY_MAP_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"

#include "vt/topos/mapping/mapping_function.h"

namespace vt { namespace auto_registry {

using namespace mapping;

AutoActiveMapFunctorType getAutoHandlerFunctorMap(HandlerType const& han);

template <typename FunctorT, typename... Args>
HandlerType makeAutoHandlerFunctorMap();

// Registration for collection index mapping functions
AutoActiveMapType getAutoHandlerMap(HandlerType const& handler);

template <typename IndexT, ActiveMapTypedFnType<IndexT>* f>
HandlerType makeAutoHandlerMap();

// Registration for seed mapping singletons
AutoActiveSeedMapType getAutoHandlerSeedMap(HandlerType const& handler);

template <ActiveSeedMapFnType* f>
HandlerType makeAutoHandlerSeedMap();

AutoActiveMapType getHandlerMap(HandlerType const& han);

}} // end namespace vt::auto_registry

#include "vt/registry/auto/map/auto_registry_map_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_MAP_AUTO_REGISTRY_MAP_H*/
