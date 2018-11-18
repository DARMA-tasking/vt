
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
