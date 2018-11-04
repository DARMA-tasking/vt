
#if !defined INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_H
#define INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/collection/active/active_funcs.h"

namespace vt { namespace auto_registry {

using namespace ::vt::vrt::collection;

AutoActiveCollectionType getAutoHandlerCollection(HandlerType const& handler);

template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT>* f>
HandlerType makeAutoHandlerCollection(MsgT* const msg);

AutoActiveCollectionMemType getAutoHandlerCollectionMem(
  HandlerType const& handler
);

template <
  typename ColT, typename MsgT, ActiveColMemberTypedFnType<MsgT, ColT> f
>
HandlerType makeAutoHandlerCollectionMem(MsgT* const msg);

}} /* end namespace vt::auto_registry */

#include "vt/registry/auto/collection/auto_registry_collection.impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_COLLECTION_AUTO_REGISTRY_COLLECTION_H*/
