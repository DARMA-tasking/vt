
#if !defined INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/reducable/reducable.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
Reducable<ColT,IndexT,BaseProxyT>::Reducable(VirtualProxyType const in_proxy)
  : BaseProxyT(in_proxy)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename OpT, typename MsgT>
EpochType Reducable<ColT,IndexT,BaseProxyT>::reduce(
  MsgT *const msg, Callback<MsgT> cb, EpochType const& epoch, TagType const& tag
) const {
  using ReduceCBType = collective::reduce::operators::ReduceCallback<MsgT>;
  auto const proxy = this->getProxy();
  msg->setCallback(cb);
  debug_print(
    reduce, node,
    "Reducable: valid={} {}, ptr={}\n", cb.valid(), msg->getCallback().valid(),
    print_ptr(msg)
  );
  auto const root_node = 0;
  return theCollection()->reduceMsg<
    ColT,
    MsgT,
    MsgT::template msgHandler<MsgT, OpT, ReduceCBType>
  >(proxy,msg,epoch,tag,root_node);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Reducable<ColT,IndexT,BaseProxyT>::reduce(
  MsgT *const msg, EpochType const& epoch, TagType const& tag,
  NodeType const& node
) const {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsg<ColT,MsgT,f>(proxy,msg,epoch,tag,node);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Reducable<ColT,IndexT,BaseProxyT>::reduceExpr(
  MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch,
  TagType const& tag, NodeType const& node
) const {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsgExpr<ColT,MsgT,f>(
    proxy,msg,fn,epoch,tag,node
  );
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Reducable<ColT,IndexT,BaseProxyT>::reduce(
  MsgT *const msg, EpochType const& epoch, TagType const& tag, IndexT const& idx
) const {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsg<ColT,MsgT,f>(proxy,msg,epoch,tag,idx);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType Reducable<ColT,IndexT,BaseProxyT>::reduceExpr(
  MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch,
  TagType const& tag, IndexT const& idx
) const {
  auto const proxy = this->getProxy();
  return theCollection()->reduceMsgExpr<ColT,MsgT,f>(
    proxy,msg,fn,epoch,tag,idx
  );
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_IMPL_H*/
