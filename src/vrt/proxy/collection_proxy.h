
#if !defined INCLUDED_VRT_PROXY_COLLECTION_PROXY_H
#define INCLUDED_VRT_PROXY_COLLECTION_PROXY_H

#include "config.h"
#include "vrt/collection/proxy_traits/proxy_col_traits.h"
#include "vrt/proxy/base_elm_proxy.h"
#include "vrt/proxy/collection_elm_proxy.h"
#include "vrt/proxy/base_collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

/*
 * `CollectionIndexProxy' (variant with IndexT baked into class, allowing
 * constructors to be forwarded for building indicies in line without the type.
 */

template <typename ColT, typename IndexT>
struct CollectionProxy : ProxyCollectionTraits<ColT, IndexT> {
  using ElmProxyType = VrtElmProxy<ColT, IndexT>;

  CollectionProxy() = default;
  CollectionProxy(CollectionProxy const&) = default;
  CollectionProxy(VirtualProxyType const in_proxy);

  CollectionProxy& operator=(CollectionProxy const&) = default;

  template <typename... IndexArgsT>
  ElmProxyType index_build(IndexArgsT&&... idx) const;
  template <typename... IndexArgsT>
  ElmProxyType operator[](IndexArgsT&&... idx) const;
  template <typename... IndexArgsT>
  ElmProxyType operator()(IndexArgsT&&... idx) const;

  ElmProxyType index(IndexT const& idx) const;
  ElmProxyType operator[](IndexT const& idx) const;
  ElmProxyType operator()(IndexT const& idx) const;
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

template <typename ColT, typename IndexT>
using CollectionIndexProxy = vrt::collection::CollectionProxy<ColT,IndexT>;

template <typename ColT, typename IndexT>
using CollectionProxy = vrt::collection::CollectionProxy<ColT,IndexT>;

} /* end namespace vt */

#include "vrt/proxy/collection_proxy.impl.h"

#endif /*INCLUDED_VRT_PROXY_COLLECTION_PROXY_H*/
