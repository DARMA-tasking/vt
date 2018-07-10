
#if !defined INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_IMPL_H

#include "config.h"
#include "vrt/collection/broadcast/broadcastable.h"
#include "vrt/collection/destroy/destroyable.h"
#include "vrt/proxy/base_wrapper.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
Broadcastable<ColT, IndexT>::Broadcastable(VirtualProxyType const in_proxy)
  : Destroyable<ColT, IndexT>(in_proxy)
{ }

template <typename ColT, typename IndexT>
template <
  typename MsgT,
  ActiveColTypedFnType<MsgT, typename MsgT::CollectionType> *f
>
void Broadcastable<ColT, IndexT>::broadcast(MsgT* msg, ActionType cont) const {
  auto proxy = this->getProxy();
  return theCollection()->broadcastMsg<MsgT, f>(proxy,msg,cont);
}

template <typename ColT, typename IndexT>
template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
>
void Broadcastable<ColT, IndexT>::broadcast(MsgT* msg, ActionType cont) const {
  auto proxy = this->getProxy();
  return theCollection()->broadcastMsg<MsgT, f>(proxy,msg,cont);
}

template <typename ColT, typename IndexT>
void Broadcastable<ColT, IndexT>::finishedInserting(ActionType action) const {
  auto const col_proxy = this->getProxy();
  theCollection()->finishedInserting<ColT,IndexT>(col_proxy,action);
}

template <typename ColT, typename IndexT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Broadcastable<ColT, IndexT>::reduce(
  MsgT *const msg, EpochType const& epoch, TagType const& tag
) {
  auto const col_proxy = this->getProxy();
  return theCollection()->reduceMsg<ColT,MsgT,f>(col_proxy,msg,epoch,tag);
}

template <typename ColT, typename IndexT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Broadcastable<ColT, IndexT>::reduceExpr(
  MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch,
  TagType const& tag
) {
  auto const col_proxy = this->getProxy();
  return theCollection()->reduceMsgExpr<ColT,MsgT,f>(col_proxy,msg,fn,epoch,tag);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_IMPL_H*/
