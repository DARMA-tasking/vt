
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/collection/holders/holder.h"
#include "vt/vrt/collection/holders/base_holder.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct CollectionHolder : BaseHolder {
  CollectionHolder(
    HandlerType const& in_map_fn, IndexT const& idx, bool const in_is_static
  );

  void destroy() override;

  bool is_static_ = false;
  HandlerType map_fn = uninitialized_handler;
  IndexT max_idx;
  Holder<ColT,IndexT> holder_;
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/holders/col_holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_COL_HOLDER_H*/
