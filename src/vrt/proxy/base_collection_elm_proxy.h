
#if !defined INCLUDED_VRT_PROXY_BASE_COLLECTION_ELM_PROXY_H
#define INCLUDED_VRT_PROXY_BASE_COLLECTION_ELM_PROXY_H

#include "config.h"
#include "vrt/proxy/base_elm_proxy.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct BaseCollectionElmProxy {
  using CollectionType = ColT;
  using IndexType = IndexT;
  using ProxyType = VirtualProxyType;
  using ElementProxyType = BaseElmProxy<ColT, IndexT>;

  BaseCollectionElmProxy() = default;
  BaseCollectionElmProxy(
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

#include "vrt/proxy/base_collection_elm_proxy.impl.h"

#endif /*INCLUDED_VRT_PROXY_BASE_COLLECTION_ELM_PROXY_H*/
