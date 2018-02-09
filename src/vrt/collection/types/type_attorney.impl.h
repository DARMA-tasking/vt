
#if !defined INCLUDED_VRT_COLLECTION_TYPES_TYPE_ATTORNEY_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_TYPE_ATTORNEY_IMPL_H

#include "config.h"
#include "vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename CollectionT>
/*static*/ void CollectionTypeAttorney::setSize(
  CollectionT const& collection, VirtualElmCountType const& elms
) {
  return collection.setSize(elms);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_TYPE_ATTORNEY_IMPL_H*/
