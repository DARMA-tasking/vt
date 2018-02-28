
#if !defined INCLUDED_VRT_COLLECTION_SEND_SENDABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_SEND_SENDABLE_IMPL_H

#include "config.h"
#include "vrt/collection/send/sendable.h"
#include "vrt/proxy/base_collection.h"
#include "vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
Sendable<IndexT>::Sendable(
  typename BaseCollectionProxy<IndexT>::ProxyType const& in_proxy,
  typename BaseCollectionProxy<IndexT>::ElementProxyType const& in_elm_proxy
) : BaseCollectionProxy<IndexT>(in_proxy, in_elm_proxy)
{ }

template <typename IndexT>
template <typename SerializerT>
void Sendable<IndexT>::serialize(SerializerT& s) {
  BaseCollectionProxy<IndexT>::serialize(s);
}

template <typename IndexT>
template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
void Sendable<IndexT>::send(MsgT* msg, ActionType continuation) {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto proxy = VrtElmProxy<IndexT>(col_proxy,elm_proxy);
  /*
   * @todo:
   *.  Directly reuse this proxy: static_cast<VrtElmProxy<IndexT>*>(this)
   */
  return theCollection()->sendMsg<ColT, MsgT, f>(proxy, msg, continuation);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_SEND_SENDABLE_IMPL_H*/
