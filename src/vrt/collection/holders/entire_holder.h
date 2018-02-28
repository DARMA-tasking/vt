
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H

#include "config.h"
#include "vrt/collection/holders/col_holder.h"

#include <unordered_map>

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct EntireHolder {
  using InnerHolder = CollectionHolder<IndexT>;
  using ProxyContainerType = std::unordered_map<VirtualProxyType, InnerHolder>;

  static ProxyContainerType proxy_container_;
};

template <typename IndexT>
/*static*/ typename EntireHolder<IndexT>::ProxyContainerType
EntireHolder<IndexT>::proxy_container_;

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H*/
