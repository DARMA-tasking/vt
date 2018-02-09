
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
  explicit StaticInsertableCollectionBase(VirtualElmCountType const inNumElems);
  StaticInsertableCollectionBase();

  static bool isStaticSized();
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/types/static_insertable.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_H*/
