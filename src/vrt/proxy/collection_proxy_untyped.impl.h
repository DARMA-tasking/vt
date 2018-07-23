
#if !defined INCLUDED_VRT_PROXY_COLLECTION_PROXY_UNTYPED_IMPL_H
#define INCLUDED_VRT_PROXY_COLLECTION_PROXY_UNTYPED_IMPL_H

#include "config.h"
#include "vrt/proxy/base_elm_proxy.h"
#include "vrt/proxy/collection_proxy_untyped.h"

namespace vt { namespace vrt { namespace collection {

/*
 * CollectionProxy implementation (w/o IndexT)
 */

template <typename ColT, typename IndexT>
CollectionUntypedProxy::ElmProxyType<ColT, IndexT>
CollectionUntypedProxy::index(IndexT const& idx) const {
  return ElmProxyType<ColT, IndexT>{proxy_,BaseElmProxy<ColT,IndexT>{idx}};
}

template <typename ColT, typename IndexT>
CollectionUntypedProxy::ElmProxyType<ColT, IndexT>
CollectionUntypedProxy::operator[](IndexT const& idx) const {
  return index<ColT, IndexT>(idx);
}

template <typename ColT, typename IndexT>
CollectionUntypedProxy::ElmProxyType<ColT, IndexT>
CollectionUntypedProxy::operator()(IndexT const& idx) const {
  return index<ColT, IndexT>(idx);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_PROXY_COLLECTION_PROXY_UNTYPED_IMPL_H*/
