
#if !defined INCLUDED_VRT_PROXY_BASE_COLLECTION_IMPL_H
#define INCLUDED_VRT_PROXY_BASE_COLLECTION_IMPL_H

#include "config.h"
#include "vrt/proxy/base_collection.h"
#include "vrt/proxy/proxy_element.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
BaseCollectionProxy<ColT, IndexT>::BaseCollectionProxy(
  ProxyType const& in_col_proxy, ElementProxyType const& in_elm_proxy
) : col_proxy_(in_col_proxy), elm_proxy_(in_elm_proxy)
{ }

template <typename ColT, typename IndexT>
template <typename SerializerT>
void BaseCollectionProxy<ColT, IndexT>::serialize(SerializerT& s) {
  s | col_proxy_ | elm_proxy_;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_PROXY_BASE_COLLECTION_IMPL_H*/
