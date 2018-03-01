
#if !defined INCLUDED_VRT_PROXY_BASE_WRAPPER_IMPL_H
#define INCLUDED_VRT_PROXY_BASE_WRAPPER_IMPL_H

#include "config.h"
#include "vrt/proxy/base_wrapper.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
BaseEntireCollectionProxy<IndexT>::BaseEntireCollectionProxy(
  VirtualProxyType const in_proxy
) : proxy_(in_proxy)
{ }

template <typename IndexT>
VirtualProxyType BaseEntireCollectionProxy<IndexT>::getProxy() const {
  return proxy_;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_PROXY_BASE_WRAPPER_IMPL_H*/
