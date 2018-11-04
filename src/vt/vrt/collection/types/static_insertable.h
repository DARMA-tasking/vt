
#if !defined INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_H

#include "vt/config.h"
#include "vt/vrt/collection/types/base.h"
#include "vt/vrt/collection/types/static_size.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct StaticInsertableCollectionBase :
  StaticCollectionBase<ColT, IndexT>,
  Insertable<ColT, IndexT>
{
  explicit StaticInsertableCollectionBase(VirtualElmCountType const inNumElems);
  StaticInsertableCollectionBase();

  static bool isStaticSized();
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/types/static_insertable.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_INSERTABLE_H*/
