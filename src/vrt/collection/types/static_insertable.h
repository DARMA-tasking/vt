
#if !defined INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_H

#include "config.h"
#include "vrt/collection/types/base.h"
#include "vrt/collection/types/static_size.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct StaticInsertableCollectionBase :
  StaticCollectionBase<IndexT>,
  Insertable<IndexT>
{
  explicit StaticInsertableCollectionBase(VirtualElmCountType const inNumElems)
    : StaticCollectionBase<IndexT>(inNumElems)
  {
    CollectionBase<IndexT>::elmsFixedAtCreation_ = false;
  }

  static bool isStaticSized() { return false; }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_H*/
