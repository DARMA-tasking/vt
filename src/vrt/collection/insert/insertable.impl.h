
#if !defined INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_IMPL_H

#include "config.h"
#include "vrt/collection/insert/insertable.h"
#include "vrt/proxy/base_collection.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
ElmInsertable<ColT, IndexT>::ElmInsertable(
  typename BaseCollectionProxy<ColT, IndexT>::ProxyType const& in_proxy,
  typename BaseCollectionProxy<ColT, IndexT>::ElementProxyType const& in_elm
) : Sendable<ColT, IndexT>(in_proxy, in_elm)
{ }

template <typename ColT, typename IndexT>
template <typename SerializerT>
void ElmInsertable<ColT, IndexT>::serialize(SerializerT& s) {
  Sendable<ColT, IndexT>::serialize(s);
}

template <typename ColT, typename IndexT>
void ElmInsertable<ColT, IndexT>::insert(IndexT max, NodeType node) {
  auto const col_proxy = this->getCollectionProxy();
  auto const elm_proxy = this->getElementProxy();
  auto const idx = elm_proxy.getIndex();
  theCollection()->insert<ColT,IndexT>(col_proxy,max,idx,node);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_IMPL_H*/
