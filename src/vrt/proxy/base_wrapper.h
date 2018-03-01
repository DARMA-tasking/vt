
#if !defined INCLUDED_VRT_PROXY_BASE_WRAPPER_H
#define INCLUDED_VRT_PROXY_BASE_WRAPPER_H

#include "config.h"
#include "vrt/vrt_common.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct BaseEntireCollectionProxy {
  BaseEntireCollectionProxy() = default;
  BaseEntireCollectionProxy(VirtualProxyType const in_proxy);

  VirtualProxyType getProxy() const;

protected:
  VirtualProxyType const proxy_ = no_vrt_proxy;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/proxy/base_wrapper.impl.h"

#endif /*INCLUDED_VRT_PROXY_BASE_WRAPPER_H*/
