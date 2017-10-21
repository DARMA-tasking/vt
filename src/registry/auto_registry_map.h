
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_MAP_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_MAP_H

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_general.h"
#include "registry.h"

#include "topos/mapping/mapping_function.h"

namespace vt { namespace auto_registry {

using namespace mapping;

// Registration for collection index mapping functions
AutoActiveMapType getAutoHandlerMap(HandlerType const& handler);

template <typename IndexT, ActiveMapTypedFnType<IndexT>* f>
HandlerType makeAutoHandlerMap();

// Registration for seed mapping singletons
AutoActiveSeedMapType getAutoHandlerSeedMap(HandlerType const& handler);

template <ActiveSeedMapFnType* f>
HandlerType makeAutoHandlerSeedMap();

}} // end namespace vt::auto_registry

#include "auto_registry_map_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_MAP_H*/
