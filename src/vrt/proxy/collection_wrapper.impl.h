
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

template <typename IndexT>
CollectionProxy::ElmProxyType<IndexT>
CollectionProxy::index(IndexT const& idx) {
  return ElmProxyType<IndexT>{proxy_,VirtualProxyElementType<IndexT>{idx}};
}

template <typename IndexT>
CollectionProxy::ElmProxyType<IndexT>
CollectionProxy::operator[](IndexT const& idx) {
  return index<IndexT>(idx);
}

template <typename IndexT>
CollectionProxy::ElmProxyType<IndexT>
CollectionProxy::operator()(IndexT const& idx) {
  return index<IndexT>(idx);
}

/*
 * CollectionIndexProxy implementation (w/IndexT)
 */

template <typename IndexT>
CollectionIndexProxy<IndexT>::CollectionIndexProxy(
  VirtualProxyType const in_proxy
) : proxy_(in_proxy)
{ }

template <typename IndexT>
VirtualProxyType CollectionIndexProxy<IndexT>::getProxy() const {
  return proxy_;
}

template <typename IndexT>
template <typename... IndexArgsT>
typename CollectionIndexProxy<IndexT>::ElmProxyType
CollectionIndexProxy<IndexT>::index_build(IndexArgsT&&... args) {
  return index(IndexT(args...));
}

template <typename IndexT>
template <typename... IndexArgsT>
typename CollectionIndexProxy<IndexT>::ElmProxyType
CollectionIndexProxy<IndexT>::operator[](IndexArgsT&&... args) {
  return index_build(std::forward<IndexArgsT>(args)...);
}

template <typename IndexT>
template <typename... IndexArgsT>
typename CollectionIndexProxy<IndexT>::ElmProxyType
CollectionIndexProxy<IndexT>::operator()(IndexArgsT&&... args) {
  return index_build(std::forward<IndexArgsT>(args)...);
}

template <typename IndexT>
typename CollectionIndexProxy<IndexT>::ElmProxyType
CollectionIndexProxy<IndexT>::index(IndexT const& idx) {
  return ElmProxyType{proxy_,VirtualProxyElementType<IndexT>{idx}};
}

template <typename IndexT>
typename CollectionIndexProxy<IndexT>::ElmProxyType
CollectionIndexProxy<IndexT>::operator[](IndexT const& idx) {
  return index(idx);
}

template <typename IndexT>
typename CollectionIndexProxy<IndexT>::ElmProxyType
CollectionIndexProxy<IndexT>::operator()(IndexT const& idx) {
  return index(idx);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_IMPL_H*/
