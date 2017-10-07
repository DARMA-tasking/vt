
#if !defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_MAP_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_MAP_IMPL__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry.h"
#include "auto_registry_map.h"
#include "mapping_function.h"

namespace vt { namespace auto_registry {

using namespace mapping;

template <typename IndexT, ActiveMapFunctionType<IndexT>* f>
inline HandlerType makeAutoHandlerMap() {
  HandlerType const id = RunnableGen<
    decltype(vt::auto_registry::FunctorAdapter<
      ActiveMapFunctionType<IndexT>, f
    >()),
    AutoActiveMapContainerType, AutoRegInfoType<AutoActiveMapType>,
    SimpleMapFunctionType
  >::idx;

  return id;
}

inline AutoActiveMapType getAutoHandlerMap(HandlerType const& handler) {
  return getAutoRegistryGen<AutoActiveMapContainerType>().at(handler).getFun();
}


}} /* end namespace vt::auto_registry */

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_MAP_IMPL__*/
