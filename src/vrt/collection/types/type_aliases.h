
#if !defined INCLUDED_VRT_COLLECTION_TYPES_TYPE_ALIASES_H
#define INCLUDED_VRT_COLLECTION_TYPES_TYPE_ALIASES_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/static_size.h"
#include "vrt/collection/types/static_insertable.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Collection :
  StaticCollectionBase<IndexT>
{
  explicit Collection(VirtualElmCountType const elms)
    : StaticCollectionBase<IndexT>(elms)
  { }
};

template <typename IndexT>
struct InsertableCollection :
  StaticInsertableCollectionBase<IndexT>
{
  explicit InsertableCollection(VirtualElmCountType const elms)
    : StaticInsertableCollectionBase<IndexT>(elms)
  { }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_TYPE_ALIASES_H*/
