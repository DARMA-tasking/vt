
#if !defined INCLUDED_VRT_COLLECTION_SEND_SENDABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_SEND_SENDABLE_IMPL_H

#include "config.h"
#include "vrt/collection/send/sendable.h"
#include "vrt/proxy/base_collection.h"
#include "vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
Sendable<ColT, IndexT>::Sendable(
  typename BaseCollectionProxy<ColT, IndexT>::ProxyType const& in_proxy,
  typename BaseCollectionProxy<ColT, IndexT>::ElementProxyType const& in_elm
) : BaseCollectionProxy<ColT, IndexT>(in_proxy, in_elm)
{ }

template <typename ColT, typename IndexT>
template <typename SerializerT>
void Sendable<ColT, IndexT>::serialize(SerializerT& s) {
  BaseCollectionProxy<ColT, IndexT>::serialize(s);
}

template <typename ColT, typename IndexT>
template <typename ColU, typename MsgT, ActiveColTypedFnType<MsgT, ColU> *f>
void Sendable<ColT, IndexT>::send(MsgT* msg, ActionType continuation) {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy,elm_proxy);
  /*
   * @todo:
   *.  Directly reuse this proxy: static_cast<VrtElmProxy<IndexT>*>(this)
   */
  return theCollection()->sendMsg<ColU, MsgT, f>(proxy, msg, continuation);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_SEND_SENDABLE_IMPL_H*/
