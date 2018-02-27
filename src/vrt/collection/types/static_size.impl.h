
#if !defined INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_IMPL_H

#include "config.h"
#include "vrt/collection/types/base.h"
#include "vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
StaticCollectionBase<IndexT>::StaticCollectionBase(
  VirtualElmCountType const inNumElems
) : CollectionBase<IndexT>(false, false), numElems_(inNumElems)
{ }

template <typename IndexT>
StaticCollectionBase<IndexT>::StaticCollectionBase()
  : StaticCollectionBase(no_elms)
{ }

template <typename IndexT>
VirtualElmCountType StaticCollectionBase<IndexT>::getSize() const {
  return numElems_;
}

template <typename IndexT>
/*static*/ bool StaticCollectionBase<IndexT>::isStaticSized() {
  return true;
}

template <typename IndexT>
void StaticCollectionBase<IndexT>::setSize(VirtualElmCountType const& elms) {
  numElems_ = elms;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_IMPL_H*/
