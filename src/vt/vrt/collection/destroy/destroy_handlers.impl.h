
#if !defined INCLUDED_VRT_COLLECTION_DESTROY_DESTROY_HANDLERS_IMPL_H
#define INCLUDED_VRT_COLLECTION_DESTROY_DESTROY_HANDLERS_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/destroy/destroy_handlers.h"
#include "vrt/collection/destroy/destroy_msg.h"
#include "vrt/collection/destroy/manager_destroy_attorney.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
/*static*/ void DestroyHandlers::destroyNow(DestroyMsg<ColT, IndexT>* msg) {
  auto const& proxy = msg->getProxy();
  return CollectionElmDestroyAttorney<ColT,IndexT>::incomingDestroy(proxy);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DESTROY_DESTROY_HANDLERS_IMPL_H*/
