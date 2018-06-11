
#if !defined INCLUDED_VRT_PROXY_BASE_WRAPPER_H
#define INCLUDED_VRT_PROXY_BASE_WRAPPER_H

#include "config.h"
#include "vrt/vrt_common.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct BaseEntireCollectionProxy {
  using CollectionType = ColT;
  using IndexType = IndexT;

  BaseEntireCollectionProxy() = default;
  BaseEntireCollectionProxy(BaseEntireCollectionProxy const&) = default;
  BaseEntireCollectionProxy(BaseEntireCollectionProxy&&) = default;
  BaseEntireCollectionProxy(VirtualProxyType const in_proxy);
  BaseEntireCollectionProxy& operator=(
    BaseEntireCollectionProxy const&
  ) = default;

  VirtualProxyType getProxy() const;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | proxy_;
  }

protected:
  VirtualProxyType proxy_ = no_vrt_proxy;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/proxy/base_wrapper.impl.h"

#endif /*INCLUDED_VRT_PROXY_BASE_WRAPPER_H*/
