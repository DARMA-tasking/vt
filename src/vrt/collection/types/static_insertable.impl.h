
#if !defined INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_IMPL_H

#include "config.h"
#include "vrt/collection/types/base.h"
#include "vrt/collection/types/static_size.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
StaticInsertableCollectionBase<IndexT>::StaticInsertableCollectionBase(
  VirtualElmCountType const inNumElems
) : StaticCollectionBase<IndexT>(inNumElems),
    Insertable<IndexT>()
{
  CollectionBase<IndexT>::elmsFixedAtCreation_ = false;
}

template <typename IndexT>
StaticInsertableCollectionBase<IndexT>::StaticInsertableCollectionBase()
  : StaticInsertableCollectionBase(no_elms)
{ }

template <typename IndexT>
/*static*/ bool StaticInsertableCollectionBase<IndexT>::isStaticSized() {
  return false;
}

}}} /* end namespace vt::vrt::collection */


#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_IMPL_H*/
