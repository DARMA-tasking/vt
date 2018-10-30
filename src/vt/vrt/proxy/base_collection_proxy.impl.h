
#if !defined INCLUDED_VRT_PROXY_BASE_COLLECTION_PROXY_IMPL_H
#define INCLUDED_VRT_PROXY_BASE_COLLECTION_PROXY_IMPL_H

#include "config.h"
#include "vrt/proxy/base_collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
BaseCollectionProxy<ColT, IndexT>::BaseCollectionProxy(
  VirtualProxyType const in_proxy
) : proxy_(in_proxy)
{ }

template <typename ColT, typename IndexT>
VirtualProxyType BaseCollectionProxy<ColT, IndexT>::getProxy() const {
  return proxy_;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_PROXY_BASE_COLLECTION_PROXY_IMPL_H*/
