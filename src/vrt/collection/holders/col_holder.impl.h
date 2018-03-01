
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_IMPL_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_IMPL_H

#include "config.h"
#include "vrt/collection/holders/col_holder.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
CollectionHolder<IndexT>::CollectionHolder(
  HandlerType const& in_map_fn, IndexT const& idx
) : map_fn(in_map_fn), max_idx(idx)
{ }

template <typename IndexT>
void CollectionHolder<IndexT>::destroy() {
  holder_.destroyAll();
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_IMPL_H*/
