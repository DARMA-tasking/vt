
#if !defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_MAP__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_MAP__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_general.h"
#include "registry.h"

#include "mapping_function.h"

namespace vt { namespace auto_registry {

using namespace mapping;

AutoActiveMapType getAutoHandlerMap(HandlerType const& handler);

template <typename IndexT, ActiveMapFunctionType<IndexT>* f>
HandlerType makeAutoHandlerMap();

}} // end namespace vt::auto_registry

#include "auto_registry_map_impl.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_MAP__*/
