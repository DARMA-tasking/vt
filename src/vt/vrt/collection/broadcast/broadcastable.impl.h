
#if !defined INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/broadcast/broadcastable.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
Broadcastable<ColT,IndexT,BaseProxyT>::Broadcastable(
  VirtualProxyType const in_proxy
) : BaseProxyT(in_proxy)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT,
  ActiveColTypedFnType<MsgT, typename MsgT::CollectionType> *f
>
void Broadcastable<ColT,IndexT,BaseProxyT>::broadcast(
  MsgT* msg, ActionType cont
) const {
  auto proxy = this->getProxy();
  return theCollection()->broadcastMsg<MsgT, f>(proxy,msg,cont);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
>
void Broadcastable<ColT,IndexT,BaseProxyT>::broadcast(
  MsgT* msg, ActionType cont
) const {
  auto proxy = this->getProxy();
  return theCollection()->broadcastMsg<MsgT, f>(proxy,msg,cont);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_IMPL_H*/
