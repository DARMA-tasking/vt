
#if !defined INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/types/base.h"
#include "vt/vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
StaticCollectionBase<ColT, IndexT>::StaticCollectionBase(
  VirtualElmCountType const inNumElems
) : CollectionBase<ColT, IndexT>(false, false, inNumElems)
{ }

template <typename ColT, typename IndexT>
StaticCollectionBase<ColT, IndexT>::StaticCollectionBase()
  : StaticCollectionBase(no_elms)
{ }

template <typename ColT, typename IndexT>
VirtualElmCountType StaticCollectionBase<ColT, IndexT>::getSize() const {
  return this->numElems_;
}

template <typename ColT, typename IndexT>
/*static*/ bool StaticCollectionBase<ColT, IndexT>::isStaticSized() {
  return true;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_IMPL_H*/
