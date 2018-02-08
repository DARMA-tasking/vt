
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_ACTIVE_FUNCS_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_ACTIVE_FUNCS_H

#include "config.h"
#include "messaging/message.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/collection_elm_proxy.h"
#include "vrt/collection/types/headers.h"

namespace vt { namespace vrt { namespace collection {

using ActiveCollectionFnPtrType = void(*)(
  ::vt::BaseMessage*, UntypedCollection*
);

template <typename MessageT, typename CollectionT>
using ActiveCollectionTypedFnType = void(MessageT*, CollectionT*);

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_ACTIVE_FUNCS_H*/
