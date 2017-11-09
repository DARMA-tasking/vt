
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_COLLECTION_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_COLLECTION_H

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_general.h"
#include "registry.h"
#include "activefn/activefn.h"
#include "vrt/collection/collection_active_funcs.h"

namespace vt { namespace auto_registry {

using namespace ::vt::vrt::collection;

AutoActiveCollectionType getAutoHandlerCollection(HandlerType const& handler);

template <
  typename CollectionT,
  typename MessageT,
  ActiveCollectionTypedFnType<MessageT, CollectionT>* f
>
HandlerType makeAutoHandlerCollection(MessageT* const msg);

}} /* end namespace vt::auto_registry */

#include "auto_registry_collection.impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_COLLECTION_H*/
