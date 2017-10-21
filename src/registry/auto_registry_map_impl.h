
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_MAP_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_MAP_IMPL_H

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry.h"
#include "auto_registry_map.h"
#include "topos/mapping/mapping_function.h"
#include "auto_registry_map.h"

namespace vt { namespace auto_registry {

using namespace mapping;

template <typename IndexT, ActiveMapTypedFnType<IndexT>* f>
inline HandlerType makeAutoHandlerMap() {
  HandlerType const id = RunnableGen<
    decltype(vt::auto_registry::FunctorAdapter<
      ActiveMapTypedFnType<IndexT>, f
    >()),
    AutoActiveMapContainerType, AutoRegInfoType<AutoActiveMapType>,
    ActiveMapFnPtrType
  >::idx;

  return id;
}

inline AutoActiveMapType getAutoHandlerMap(HandlerType const& handler) {
  return getAutoRegistryGen<AutoActiveMapContainerType>().at(handler).getFun();
}

template <ActiveSeedMapFnType* f>
inline HandlerType makeAutoHandlerSeedMap() {
  HandlerType const id = RunnableGen<
    decltype(vt::auto_registry::FunctorAdapter<
      ActiveSeedMapFnType, f
    >()),
    AutoActiveSeedMapContainerType, AutoRegInfoType<AutoActiveSeedMapType>,
    ActiveSeedMapFnPtrType
  >::idx;

  return id;
}

// Registration for seed mapping singletons
inline AutoActiveSeedMapType getAutoHandlerSeedMap(HandlerType const& handler) {
  return getAutoRegistryGen<AutoActiveSeedMapContainerType>().at(handler).getFun();
}

}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_MAP_IMPL_H*/
