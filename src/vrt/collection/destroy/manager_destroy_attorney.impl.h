
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_DESTROY_ATTORNEY_IMPL_H
#define INCLUDED_VRT_COLLECTION_MANAGER_DESTROY_ATTORNEY_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/destroy/manager_destroy_attorney.h"
#include "vrt/collection/manager.fwd.h"

#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
/*static*/ void CollectionElmDestroyAttorney<ColT, IndexT>::incomingDestroy(
  VirtualProxyType const& proxy
) {
  return theCollection()->incomingDestroy<ColT,IndexT>(proxy);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_DESTROY_ATTORNEY_IMPL_H*/
