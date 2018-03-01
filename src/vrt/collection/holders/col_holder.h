
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_H

#include "config.h"
#include "vrt/collection/holders/holder.h"
#include "vrt/collection/holders/base_holder.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct CollectionHolder : BaseHolder {
  CollectionHolder(HandlerType const& in_map_fn, IndexT const& idx);

  void destroy() override;

  HandlerType map_fn = uninitialized_handler;
  IndexT max_idx;
  Holder<IndexT> holder_;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/holders/col_holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_H*/
