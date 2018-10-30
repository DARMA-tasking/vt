
#if !defined INCLUDED_VRT_PROXY_BASE_COLLECTION_PROXY_H
#define INCLUDED_VRT_PROXY_BASE_COLLECTION_PROXY_H

#include "config.h"
#include "vrt/vrt_common.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct BaseCollectionProxy {
  using CollectionType = ColT;
  using IndexType = IndexT;

  BaseCollectionProxy() = default;
  BaseCollectionProxy(BaseCollectionProxy const&) = default;
  BaseCollectionProxy(BaseCollectionProxy&&) = default;
  explicit BaseCollectionProxy(VirtualProxyType const in_proxy);
  BaseCollectionProxy& operator=(
    BaseCollectionProxy const&
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

#include "vrt/proxy/base_collection_proxy.impl.h"

#endif /*INCLUDED_VRT_PROXY_BASE_COLLECTION_PROXY_H*/
