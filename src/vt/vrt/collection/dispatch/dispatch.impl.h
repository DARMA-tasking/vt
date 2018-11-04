
#if !defined INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_IMPL_H
#define INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/dispatch/dispatch.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/vrt/collection/traits/coll_msg.h"
#include "vt/vrt/collection/manager.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename MsgT>
void DispatchCollection<ColT,MsgT>::broadcast(
  VirtualProxyType proxy, void* msg, HandlerType han, bool member,
  ActionType action
) {
  using IdxT = typename ColT::IndexType;
  auto const msg_typed = reinterpret_cast<MsgT*>(msg);
  CollectionProxy<ColT,IdxT> typed_proxy{proxy};
  theCollection()->broadcastMsgWithHan<MsgT,ColT>(
    typed_proxy,msg_typed,han,member,action,true
  );
}

template <typename ColT, typename MsgT>
void DispatchCollection<ColT,MsgT>::send(
  VirtualProxyType proxy, void* idx, void* msg, HandlerType han, bool member,
  ActionType action
) {
  using IdxT = typename ColT::IndexType;
  auto const msg_typed = reinterpret_cast<MsgT*>(msg);
  auto const idx_typed = reinterpret_cast<IdxT*>(idx);
  auto const& idx_typed_ref = *idx_typed;
  VrtElmProxy<ColT,IdxT> typed_proxy{proxy,idx_typed_ref};
  theCollection()->sendMsgWithHan<MsgT,ColT>(
    typed_proxy,msg_typed,han,member,action
  );
}

template <typename always_void_>
VirtualProxyType DispatchCollectionBase::getDefaultProxy() const {
  assert(default_proxy_ != no_vrt_proxy && "Must be valid proxy");
  return default_proxy_;
}

template <typename always_void_>
void DispatchCollectionBase::setDefaultProxy(VirtualProxyType const& proxy) {
  default_proxy_ = proxy;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_IMPL_H*/
