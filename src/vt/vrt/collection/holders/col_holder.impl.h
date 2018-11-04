
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_IMPL_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/holders/col_holder.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
CollectionHolder<ColT, IndexT>::CollectionHolder(
  HandlerType const& in_map_fn, IndexT const& idx, bool const in_is_static
) : is_static_(in_is_static), map_fn(in_map_fn), max_idx(idx)
{ }

template <typename ColT, typename IndexT>
void CollectionHolder<ColT, IndexT>::destroy() {
  holder_.destroyAll();
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_IMPL_H*/
