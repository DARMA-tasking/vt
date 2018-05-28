
#if !defined INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_IMPL_H
#define INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_IMPL_H

#include "config.h"
#include "vrt/proxy/proxy_element.h"
#include "vrt/proxy/proxy_collection.h"
#include "vrt/proxy/collection_wrapper.h"

namespace vt { namespace vrt { namespace collection {

/*
 * CollectionProxy implementation (w/o IndexT)
 */

template <typename ColT, typename IndexT>
CollectionProxy::ElmProxyType<ColT, IndexT>
CollectionProxy::index(IndexT const& idx) {
  return ElmProxyType<ColT, IndexT>{
    proxy_,VirtualProxyElementType<ColT, IndexT>{idx}
  };
}

template <typename ColT, typename IndexT>
CollectionProxy::ElmProxyType<ColT, IndexT>
CollectionProxy::operator[](IndexT const& idx) {
  return index<ColT, IndexT>(idx);
}

template <typename ColT, typename IndexT>
CollectionProxy::ElmProxyType<ColT, IndexT>
CollectionProxy::operator()(IndexT const& idx) {
  return index<ColT, IndexT>(idx);
}

/*
 * CollectionIndexProxy implementation (w/IndexT)
 */

template <typename ColT, typename IndexT>
CollectionIndexProxy<ColT, IndexT>::CollectionIndexProxy(
  VirtualProxyType const in_proxy
) : Broadcastable<ColT, IndexT>(in_proxy)
{ }

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionIndexProxy<ColT, IndexT>::ElmProxyType
CollectionIndexProxy<ColT, IndexT>::index_build(IndexArgsT&&... args) {
  return index(IndexT(args...));
}

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionIndexProxy<ColT, IndexT>::ElmProxyType
CollectionIndexProxy<ColT, IndexT>::operator[](IndexArgsT&&... args) {
  return index_build(std::forward<IndexArgsT>(args)...);
}

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionIndexProxy<ColT, IndexT>::ElmProxyType
CollectionIndexProxy<ColT, IndexT>::operator()(IndexArgsT&&... args) {
  return index_build(std::forward<IndexArgsT>(args)...);
}

template <typename ColT, typename IndexT>
typename CollectionIndexProxy<ColT, IndexT>::ElmProxyType
CollectionIndexProxy<ColT, IndexT>::index(IndexT const& idx) {
  return ElmProxyType{this->proxy_,VirtualProxyElementType<ColT, IndexT>{idx}};
}

template <typename ColT, typename IndexT>
typename CollectionIndexProxy<ColT, IndexT>::ElmProxyType
CollectionIndexProxy<ColT, IndexT>::operator[](IndexT const& idx) {
  return index(idx);
}

template <typename ColT, typename IndexT>
typename CollectionIndexProxy<ColT, IndexT>::ElmProxyType
CollectionIndexProxy<ColT, IndexT>::operator()(IndexT const& idx) {
  return index(idx);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_IMPL_H*/
