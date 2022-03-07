/*
//@HEADER
// *****************************************************************************
//
//                           auto_registry_map_impl.h
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

#if !defined INCLUDED_VT_REGISTRY_AUTO_MAP_AUTO_REGISTRY_MAP_IMPL_H
#define INCLUDED_VT_REGISTRY_AUTO_MAP_AUTO_REGISTRY_MAP_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry.h"
#include "vt/registry/auto/map/auto_registry_map.h"
#include "vt/registry/auto/functor/auto_registry_functor.h"
#include "vt/topos/mapping/mapping_function.h"
#include "vt/registry/auto/map/auto_registry_map.h"

#include <cassert>

namespace vt { namespace auto_registry {

using namespace mapping;

/*
 * Functor map variants
 */

template <typename FunctorT, typename... Args>
inline HandlerType makeAutoHandlerFunctorMap() {
  using ContainerType = AutoActiveMapFunctorContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveMapFunctorType>;
  using FuncType = ActiveMapFnPtrType;
  using RunnableT = RunnableFunctor<
    FunctorT, ContainerType, RegInfoType, FuncType, true, Args...
  >;

  constexpr bool is_auto = true;
  constexpr bool is_functor = true;
  constexpr auto reg_type = RegistryTypeEnum::RegMap;
  auto const han = HandlerManagerType::makeHandler(
    is_auto, is_functor, RunnableT::idx, reg_type
  );
  vt_debug_print(
    verbose, handler,
    "makeAutoHandlerFunctorMap: handler={}\n", han
  );
  return han;
}

inline AutoActiveMapType const&
getAutoHandlerFunctorMap(HandlerType const han) {
  using ContainerType = AutoActiveMapFunctorContainerType;
  auto const id = HandlerManagerType::getHandlerIdentifier(han);
  bool const is_auto = HandlerManagerType::isHandlerAuto(han);
  bool const is_functor = HandlerManagerType::isHandlerFunctor(han);

  vt_debug_print(
    verbose, handler,
    "getAutoHandlerFunctorMap: handler={}, id={}, is_auto={}, is_functor={}\n",
    han, id, print_bool(is_auto), print_bool(is_functor)
  );

  assert((is_functor && is_auto) && "Handler should be auto and functor type!");

  return getAutoRegistryGen<ContainerType>().at(id).getFun();
}

/*
 * Function map variants
 */

template <typename IndexT, ActiveMapTypedFnType<IndexT>* f>
inline HandlerType makeAutoHandlerMap() {
  using FunctorType = FunctorAdapter<ActiveMapTypedFnType<IndexT>, f, IndexT>;
  using ContainerType = AutoActiveMapContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveMapType>;
  using FuncType = ActiveMapFnPtrType;

  constexpr bool is_auto = true;
  constexpr bool is_functor = false;
  auto id = RunnableGen<FunctorType, ContainerType, RegInfoType, FuncType>::idx;
  constexpr auto reg_type = RegistryTypeEnum::RegMap;
  auto const han = HandlerManagerType::makeHandler(
    is_auto, is_functor, id, reg_type
  );
  vt_debug_print(
    verbose, handler,
    "makeAutoHandlerMap: id={}, han={}\n", id, han
  );
  return han;
}

inline AutoActiveMapType const& getAutoHandlerMap(HandlerType const handler) {
  using ContainerType = AutoActiveMapContainerType;
  auto const id = HandlerManagerType::getHandlerIdentifier(handler);
  vt_debug_print(
    verbose, handler, "getAutoHandlerMap: id={}, handler={}\n", id, handler
  );
  return getAutoRegistryGen<ContainerType>().at(id).getFun();
}

template <ActiveSeedMapFnType* f>
inline HandlerType makeAutoHandlerSeedMap() {
  using FunctorType = FunctorAdapter<ActiveSeedMapFnType, f>;
  using ContainerType = AutoActiveSeedMapContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveSeedMapType>;
  using FuncType = ActiveSeedMapFnPtrType;

  constexpr bool is_auto = true;
  constexpr bool is_functor = false;
  auto id = RunnableGen<FunctorType, ContainerType, RegInfoType, FuncType>::idx;
  constexpr auto reg_type = RegistryTypeEnum::RegSeed;
  auto const han = HandlerManagerType::makeHandler(
    is_auto, is_functor, id, reg_type
  );
  vt_debug_print(
    verbose, handler,
    "makeAutoHandlerMap: id={}, han={}\n", id, han
  );
  return han;
}

// Registration for seed mapping singletons
inline AutoActiveSeedMapType getAutoHandlerSeedMap(HandlerType const handler) {
  using ContainerType = AutoActiveSeedMapContainerType;
  auto const id = HandlerManagerType::getHandlerIdentifier(handler);
  vt_debug_print(
    verbose, handler,
    "getAutoHandlerSeedMap: id={}, handler={}\n", id, handler
  );
  return getAutoRegistryGen<ContainerType>().at(id).getFun();
}

inline AutoActiveMapType const& getHandlerMap(HandlerType const han) {
  bool const is_functor = HandlerManagerType::isHandlerFunctor(han);
  if (is_functor) {
    return getAutoHandlerFunctorMap(han);
  } else {
    return getAutoHandlerMap(han);
  }
}

}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_VT_REGISTRY_AUTO_MAP_AUTO_REGISTRY_MAP_IMPL_H*/
