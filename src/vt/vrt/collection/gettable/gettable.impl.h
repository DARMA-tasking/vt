
#if !defined INCLUDED_VRT_COLLECTION_GETTABLE_GETTABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_GETTABLE_GETTABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/insert/insertable.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
Gettable<ColT,IndexT,BaseProxyT>::Gettable(
  typename BaseProxyT::ProxyType const& in_proxy,
  typename BaseProxyT::ElementProxyType const& in_elm
) : BaseProxyT(in_proxy, in_elm)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename SerializerT>
void Gettable<ColT,IndexT,BaseProxyT>::serialize(SerializerT& s) {
  BaseProxyT::serialize(s);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
ColT* Gettable<ColT,IndexT,BaseProxyT>::tryGetLocalPtr() const {
  auto const col_proxy = this->getCollectionProxy();
  auto const elm_proxy = this->getElementProxy();
  auto const idx = elm_proxy.getIndex();
  return theCollection()->tryGetLocalPtr<ColT,IndexT>(col_proxy,idx);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_GETTABLE_GETTABLE_IMPL_H*/
