
#if !defined INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/insert/insertable.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
ElmInsertable<ColT,IndexT,BaseProxyT>::ElmInsertable(
  typename BaseProxyT::ProxyType const& in_proxy,
  typename BaseProxyT::ElementProxyType const& in_elm
) : BaseProxyT(in_proxy, in_elm)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename SerializerT>
void ElmInsertable<ColT,IndexT,BaseProxyT>::serialize(SerializerT& s) {
  BaseProxyT::serialize(s);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
void ElmInsertable<ColT,IndexT,BaseProxyT>::insert(NodeType node) const {
  auto const col_proxy = this->getCollectionProxy();
  auto const elm_proxy = this->getElementProxy();
  auto const idx = elm_proxy.getIndex();
  theCollection()->insert<ColT,IndexT>(col_proxy,idx,node);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_IMPL_H*/
