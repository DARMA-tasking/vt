
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_H

#include "config.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct CollectionHolder {
  HandlerType map_fn = uninitialized_handler;
  IndexT max_idx;

  CollectionHolder(HandlerType const& in_map_fn, IndexT const& idx);
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/holders/col_holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_H*/
