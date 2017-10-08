
#if !defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_MAP__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_MAP__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_general.h"
#include "registry.h"

#include "mapping_function.h"

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

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_MAP__*/
