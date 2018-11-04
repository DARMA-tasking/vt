
#if !defined INCLUDED_VRT_COLLECTION_TYPES_DYNAMIC_H
#define INCLUDED_VRT_COLLECTION_TYPES_DYNAMIC_H

#include "vt/config.h"
#include "vt/vrt/collection/types/base.h"
#include "vt/vrt/collection/types/insertable_epoch.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct DynamicCollectionBase :
  CollectionBase<ColT, IndexT>,
  InsertableEpoch<ColT, IndexT>
{
  DynamicCollectionBase()
    : CollectionBase<ColT, IndexT>(false, false)
  { }

  // Unknown so return no_elms as the size
  VirtualElmCountType getSize() const { return no_elms; }

  static bool isStaticSized() { return false; }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_DYNAMIC_H*/
