
#if !defined INCLUDED_VRT_PROXY_COLLECTION_PROXY_UNTYPED_H
#define INCLUDED_VRT_PROXY_COLLECTION_PROXY_UNTYPED_H

#include "config.h"
#include "vrt/collection/proxy_traits/proxy_col_traits.h"
#include "vrt/proxy/base_elm_proxy.h"
#include "vrt/proxy/collection_elm_proxy.h"
#include "vrt/proxy/base_collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

/*
 * `CollectionUntypedProxy': variant w/o IndexT baked into class. Thus, all
 * accesses require that IndexT be determinable (so the index cannot be created
 * on the fly).
 */

struct CollectionUntypedProxy {
  template <typename ColT, typename IndexT>
  using ElmProxyType = VrtElmProxy<ColT, IndexT>;

  CollectionUntypedProxy() = default;
  CollectionUntypedProxy(VirtualProxyType const in_proxy);

  VirtualProxyType getProxy() const;

  template <typename ColT, typename IndexT>
  ElmProxyType<ColT, IndexT> index(IndexT const& idx) const;
  template <typename ColT, typename IndexT>
  ElmProxyType<ColT, IndexT> operator[](IndexT const& idx) const;
  template <typename ColT, typename IndexT>
  ElmProxyType<ColT, IndexT> operator()(IndexT const& idx) const;

private:
  VirtualProxyType const proxy_ = no_vrt_proxy;
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

using CollectionProxy = vrt::collection::CollectionUntypedProxy;

} /* end namespace vt */

#include "vrt/proxy/collection_proxy_untyped.impl.h"

#endif /*INCLUDED_VRT_PROXY_COLLECTION_PROXY_UNTYPED_H*/
