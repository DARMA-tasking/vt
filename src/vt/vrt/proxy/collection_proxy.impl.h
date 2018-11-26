
#if !defined INCLUDED_VRT_PROXY_COLLECTION_PROXY_IMPL_H
#define INCLUDED_VRT_PROXY_COLLECTION_PROXY_IMPL_H

#include "vt/config.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/vrt/proxy/base_elm_proxy.h"
#include "vt/vrt/collection/proxy_traits/proxy_col_traits.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
CollectionProxy<ColT, IndexT>::CollectionProxy(
  VirtualProxyType const in_proxy
) : ProxyCollectionTraits<ColT, IndexT>(in_proxy)
{ }

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::index_build(IndexArgsT&&... args) const {
  using BaseIndexType = typename IndexT::DenseIndexType;
  return index(IndexT(static_cast<BaseIndexType>(args)...));
}

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator[](IndexArgsT&&... args) const {
  return index_build(std::forward<IndexArgsT>(args)...);
}

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator()(IndexArgsT&&... args) const {
  return index_build(std::forward<IndexArgsT>(args)...);
}

template <typename ColT, typename IndexT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::index(IndexT const& idx) const {
  return ElmProxyType{this->proxy_,BaseElmProxy<ColT, IndexT>{idx}};
}

template <typename ColT, typename IndexT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator[](IndexT const& idx) const {
  return index(idx);
}

template <typename ColT, typename IndexT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator()(IndexT const& idx) const {
  return index(idx);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_PROXY_COLLECTION_PROXY_IMPL_H*/
