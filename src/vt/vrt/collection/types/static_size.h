
#if !defined INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_H
#define INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_H

#include "vt/config.h"
#include "vt/vrt/collection/types/base.h"
#include "vt/vrt/collection/types/type_attorney.h"
#include "vt/vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct StaticCollectionBase :
  CollectionBase<ColT, IndexT>
{
  explicit StaticCollectionBase(VirtualElmCountType const inNumElems);
  StaticCollectionBase();

  VirtualElmCountType getSize() const;
  static bool isStaticSized();

  friend struct CollectionTypeAttorney;
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/types/static_size.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_H*/
