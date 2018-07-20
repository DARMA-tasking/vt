
#if !defined INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_IMPL_H

#include "config.h"
#include "vrt/collection/reducable/reducable.h"
#include "vrt/proxy/base_wrapper.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
Reducable<ColT,IndexT>::Reducable(VirtualProxyType const in_proxy)
  :  Broadcastable<ColT,IndexT>(in_proxy)
{ }

template <typename ColT, typename IndexT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Reducable<ColT,IndexT>::reduce(
  MsgT *const msg, EpochType const& epoch, TagType const& tag,
  NodeType const& node
) {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsg<ColT,MsgT,f>(proxy,msg,epoch,tag,node);
}

template <typename ColT, typename IndexT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Reducable<ColT,IndexT>::reduceExpr(
  MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch,
  TagType const& tag, NodeType const& node
) {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsgExpr<ColT,MsgT,f>(
    proxy,msg,fn,epoch,tag,node
  );
}

template <typename ColT, typename IndexT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Reducable<ColT,IndexT>::reduce(
  MsgT *const msg, EpochType const& epoch, TagType const& tag, IndexT const& idx
) {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsg<ColT,MsgT,f>(proxy,msg,epoch,tag,idx);
}

template <typename ColT, typename IndexT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Reducable<ColT,IndexT>::reduceExpr(
  MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch,
  TagType const& tag, IndexT const& idx
) {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsgExpr<ColT,MsgT,f>(
    proxy,msg,fn,epoch,tag,idx
  );
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_IMPL_H*/
