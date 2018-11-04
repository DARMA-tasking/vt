
#if !defined INCLUDED_VRT_COLLECTION_SEND_SENDABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_SEND_SENDABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/send/sendable.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
Sendable<ColT,IndexT,BaseProxyT>::Sendable(
  typename BaseProxyT::ProxyType const& in_proxy,
  typename BaseProxyT::ElementProxyType const& in_elm
) : BaseProxyT(in_proxy, in_elm)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename SerializerT>
void Sendable<ColT,IndexT,BaseProxyT>::serialize(SerializerT& s) {
  BaseProxyT::serialize(s);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT,
  ActiveColTypedFnType<MsgT, typename MsgT::CollectionType> *f
>
void Sendable<ColT,IndexT,BaseProxyT>::send(
  MsgT* msg, ActionType continuation
) {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy,elm_proxy);
  /*
   * @todo:
   *.  Directly reuse this proxy: static_cast<VrtElmProxy<IndexT>*>(this)
   */
  return theCollection()->sendMsg<MsgT, f>(proxy, msg, continuation);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
>
void Sendable<ColT,IndexT,BaseProxyT>::send(
  MsgT* msg, ActionType continuation
) {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy,elm_proxy);
  /*
   * @todo:
   *.  Directly reuse this proxy: static_cast<VrtElmProxy<IndexT>*>(this)
   */
  return theCollection()->sendMsg<MsgT, f>(proxy, msg, continuation);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_SEND_SENDABLE_IMPL_H*/
