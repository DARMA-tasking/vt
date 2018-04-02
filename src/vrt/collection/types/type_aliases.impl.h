
#if !defined INCLUDED_VRT_COLLECTION_TYPES_TYPE_ALIASES_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_TYPE_ALIASES_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/static_size.h"
#include "vrt/collection/types/static_insertable.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
Collection<ColT, IndexT>::Collection(VirtualElmCountType const elms)
  : StaticCollectionBase<ColT, IndexT>(elms)
{ }

template <typename ColT, typename IndexT>
InsertableCollection<ColT, IndexT>::InsertableCollection(
  VirtualElmCountType const elms
) : StaticInsertableCollectionBase<ColT, IndexT>(elms)
{ }


}}} /* end namespace vt::vrt::collection */


#endif /*INCLUDED_VRT_COLLECTION_TYPES_TYPE_ALIASES_IMPL_H*/
