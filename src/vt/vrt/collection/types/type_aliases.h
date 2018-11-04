
#if !defined INCLUDED_VRT_COLLECTION_TYPES_TYPE_ALIASES_H
#define INCLUDED_VRT_COLLECTION_TYPES_TYPE_ALIASES_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/types/static_size.h"
#include "vt/vrt/collection/types/static_insertable.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Collection :
  StaticCollectionBase<ColT, IndexT>
{
  explicit Collection(VirtualElmCountType const elms = no_elms);
};

template <typename ColT, typename IndexT>
struct InsertableCollection :
  StaticInsertableCollectionBase<ColT, IndexT>
{
  explicit InsertableCollection(VirtualElmCountType const elms = no_elms);
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

template <typename ColT, typename IndexT>
using Collection = vrt::collection::Collection<ColT,IndexT>;

template <typename ColT, typename IndexT>
using InsertableCollection = vrt::collection::InsertableCollection<ColT,IndexT>;

} /* end namespace vt */

#include "vt/vrt/collection/types/type_aliases.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_TYPE_ALIASES_H*/
