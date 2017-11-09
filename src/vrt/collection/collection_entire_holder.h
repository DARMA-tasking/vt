
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_ENTIRE_HOLDER_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_ENTIRE_HOLDER_H

#include "config.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct CollectionEntireHolder {
  struct InnerHolder {
    HandlerType map_fn = uninitialized_handler;
    IndexT max_idx;

    InnerHolder(HandlerType const& in_map_fn, IndexT const& idx)
      : map_fn(in_map_fn), max_idx(idx)
    { }
  };

  using ProxyContainerType = std::unordered_map<VirtualProxyType, InnerHolder>;

  static ProxyContainerType proxy_container_;
};

template <typename IndexT>
/*static*/ typename CollectionEntireHolder<IndexT>::ProxyContainerType
CollectionEntireHolder<IndexT>::proxy_container_;

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_ENTIRE_HOLDER_H*/
