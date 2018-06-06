
#if !defined INCLUDED_VRT_PROXY_BASE_COLLECTION_H
#define INCLUDED_VRT_PROXY_BASE_COLLECTION_H

#include "config.h"
#include "vrt/proxy/proxy_element.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct BaseCollectionProxy {
  using CollectionType = ColT;
  using IndexType = IndexT;
  using ProxyType = VirtualProxyType;
  using ElementProxyType = VirtualProxyElementType<ColT, IndexT>;

  BaseCollectionProxy() = default;
  BaseCollectionProxy(
    ProxyType const& in_col_proxy, ElementProxyType const& in_elm_proxy
  );

  template <typename SerializerT>
  void serialize(SerializerT& s);

  ProxyType getCollectionProxy() const { return col_proxy_; }
  ElementProxyType getElementProxy() const { return elm_proxy_; }

protected:
  ProxyType col_proxy_ = no_vrt_proxy;
  ElementProxyType elm_proxy_{virtual_proxy_elm_empty_tag};
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/proxy/base_collection.impl.h"

#endif /*INCLUDED_VRT_PROXY_BASE_COLLECTION_H*/
