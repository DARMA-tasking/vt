
#if !defined INCLUDED_REGISTRY_AUTO_MAP_AUTO_REGISTRY_MAP_IMPL_H
#define INCLUDED_REGISTRY_AUTO_MAP_AUTO_REGISTRY_MAP_IMPL_H

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
  auto const& han = HandlerManagerType::makeHandler(true, true, RunnableT::idx);
  debug_print(
    handler, node,
    "makeAutoHandlerFunctorMap: handler={}\n", han
  );
  return han;
}

inline AutoActiveMapFunctorType getAutoHandlerFunctorMap(
  HandlerType const& han
) {
  using ContainerType = AutoActiveMapFunctorContainerType;
  auto const& id = HandlerManagerType::getHandlerIdentifier(han);
  bool const& is_auto = HandlerManagerType::isHandlerAuto(han);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(han);

  debug_print(
    handler, node,
    "getAutoHandlerFunctorMap: handler={}, id={}, is_auto={}, is_functor={}\n",
    han, id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    (is_functor && is_auto) && "Handler should be auto and functor type!"
  );

  return getAutoRegistryGen<ContainerType>().at(id).getFun();
}

/*
 * Function map variants
 */

template <typename IndexT, ActiveMapTypedFnType<IndexT>* f>
inline HandlerType makeAutoHandlerMap() {
  using FunctorType = FunctorAdapter<ActiveMapTypedFnType<IndexT>, f>;
  using ContainerType = AutoActiveMapContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveMapType>;
  using FuncType = ActiveMapFnPtrType;
  auto id = RunnableGen<FunctorType, ContainerType, RegInfoType, FuncType>::idx;
  auto const& han = HandlerManagerType::makeHandler(true,false,id);
  debug_print(
    handler, node,
    "makeAutoHandlerMap: id={}, han={}\n", id, han
  );
  return han;
}

inline AutoActiveMapType getAutoHandlerMap(HandlerType const& handler) {
  using ContainerType = AutoActiveMapContainerType;
  auto const& id = HandlerManagerType::getHandlerIdentifier(handler);
  debug_print(
    handler, node,
    "getAutoHandlerMap: id={}, handler={}\n", id, handler
  );
  return getAutoRegistryGen<ContainerType>().at(id).getFun();
}

template <ActiveSeedMapFnType* f>
inline HandlerType makeAutoHandlerSeedMap() {
  using FunctorType = FunctorAdapter<ActiveSeedMapFnType, f>;
  using ContainerType = AutoActiveSeedMapContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveSeedMapType>;
  using FuncType = ActiveSeedMapFnPtrType;
  auto id = RunnableGen<FunctorType, ContainerType, RegInfoType, FuncType>::idx;
  auto const& han = HandlerManagerType::makeHandler(true,false,id);
  debug_print(
    handler, node,
    "makeAutoHandlerMap: id={}, han={}\n", id, han
  );
  return id;
}

// Registration for seed mapping singletons
inline AutoActiveSeedMapType getAutoHandlerSeedMap(HandlerType const& handler) {
  using ContainerType = AutoActiveSeedMapContainerType;
  auto const& id = HandlerManagerType::getHandlerIdentifier(handler);
  debug_print(
    handler, node,
    "getAutoHandlerSeedMap: id={}, handler={}\n", id, handler
  );
  return getAutoRegistryGen<ContainerType>().at(id).getFun();
}

inline AutoActiveMapType getHandlerMap(HandlerType const& han) {
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(han);
  if (is_functor) {
    return getAutoHandlerFunctorMap(han);
  } else {
    return getAutoHandlerMap(han);
  }
}

}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_REGISTRY_AUTO_MAP_AUTO_REGISTRY_MAP_IMPL_H*/
