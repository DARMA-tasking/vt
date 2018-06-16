
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_DESTROY_ATTORNEY_H
#define INCLUDED_VRT_COLLECTION_MANAGER_DESTROY_ATTORNEY_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/destroy/destroy_handlers.fwd.h"

#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct CollectionElmDestroyAttorney {
  friend struct DestroyHandlers;

private:
  static void incomingDestroy(CollectionIndexProxy<ColT, IndexT> const& proxy);
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/destroy/manager_destroy_attorney.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_DESTROY_ATTORNEY_H*/
