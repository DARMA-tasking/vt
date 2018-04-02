
#if !defined INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_IMPL_H

#include "config.h"
#include "vrt/collection/types/base.h"
#include "vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
StaticCollectionBase<ColT, IndexT>::StaticCollectionBase(
  VirtualElmCountType const inNumElems
) : CollectionBase<ColT, IndexT>(false, false), numElems_(inNumElems)
{ }

template <typename ColT, typename IndexT>
StaticCollectionBase<ColT, IndexT>::StaticCollectionBase()
  : StaticCollectionBase(no_elms)
{ }

template <typename ColT, typename IndexT>
VirtualElmCountType StaticCollectionBase<ColT, IndexT>::getSize() const {
  return numElems_;
}

template <typename ColT, typename IndexT>
/*static*/ bool StaticCollectionBase<ColT, IndexT>::isStaticSized() {
  return true;
}

template <typename ColT, typename IndexT>
void StaticCollectionBase<ColT, IndexT>::setSize(
  VirtualElmCountType const& elms
) {
  numElems_ = elms;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_IMPL_H*/
