
#if !defined INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H
#define INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H

#include "config.h"
#include "vrt/proxy/base_collection_proxy.h"
#include "activefn/activefn.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct Broadcastable : BaseProxyT {
  Broadcastable() = default;
  Broadcastable(Broadcastable const&) = default;
  Broadcastable(Broadcastable&&) = default;
  Broadcastable(VirtualProxyType const in_proxy);
  Broadcastable& operator=(Broadcastable const&) = default;

  template <
    typename MsgT,
    ActiveColTypedFnType<MsgT, typename MsgT::CollectionType> *f
  >
  void broadcast(MsgT* msg, ActionType act = nullptr) const;

  template <
    typename MsgT,
    ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
  >
  void broadcast(MsgT* msg, ActionType act = nullptr) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H*/
