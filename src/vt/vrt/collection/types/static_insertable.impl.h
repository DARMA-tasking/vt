
#if !defined INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/types/base.h"
#include "vt/vrt/collection/types/static_size.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
StaticInsertableCollectionBase<ColT, IndexT>::StaticInsertableCollectionBase(
  VirtualElmCountType const inNumElems
) : StaticCollectionBase<ColT, IndexT>(inNumElems),
    Insertable<ColT, IndexT>()
{
  CollectionBase<ColT, IndexT>::elmsFixedAtCreation_ = false;
}

template <typename ColT, typename IndexT>
StaticInsertableCollectionBase<ColT, IndexT>::StaticInsertableCollectionBase()
  : StaticInsertableCollectionBase(no_elms)
{ }

template <typename ColT, typename IndexT>
/*static*/ bool StaticInsertableCollectionBase<ColT, IndexT>::isStaticSized() {
  return false;
}

}}} /* end namespace vt::vrt::collection */


#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_IMPL_H*/
