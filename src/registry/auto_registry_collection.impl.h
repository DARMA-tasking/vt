
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_COLLECTION_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_COLLECTION_IMPL_H

#include "config.h"
#include "registry/auto_registry_collection.h"

namespace vt { namespace auto_registry {

inline AutoActiveCollectionType getAutoHandlerCollection(
  HandlerType const& handler
) {
  return getAutoRegistryGen<
    AutoActiveCollectionContainerType
  >().at(handler).getFun();
}

template <
  typename CollectionT,
  typename MessageT,
  ActiveCollectionTypedFnType<MessageT, CollectionT>* f
>
inline HandlerType makeAutoHandlerCollection(MessageT* const msg) {
  HandlerType const id = RunnableGen<decltype(vt::auto_registry::FunctorAdapter<
      ActiveCollectionTypedFnType<MessageT, CollectionT>, f
    >()),
    AutoActiveCollectionContainerType,
    AutoRegInfoType<AutoActiveCollectionType>,
    ActiveCollectionFnPtrType
  >::idx;

  return id;
}

}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_COLLECTION_IMPL_H*/
