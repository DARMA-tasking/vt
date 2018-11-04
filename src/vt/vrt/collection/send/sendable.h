
#if !defined INCLUDED_VRT_COLLECTION_SEND_SENDABLE_H
#define INCLUDED_VRT_COLLECTION_SEND_SENDABLE_H

#include "vt/config.h"
#include "vt/vrt/collection/active/active_funcs.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct Sendable : BaseProxyT {
  Sendable() = default;
  Sendable(
    typename BaseProxyT::ProxyType const& in_proxy,
    typename BaseProxyT::ElementProxyType const& in_elm
  );

  template <typename SerializerT>
  void serialize(SerializerT& s);

  template <
    typename MsgT,
    ActiveColTypedFnType<MsgT, typename MsgT::CollectionType> *f
  >
  void send(MsgT* msg, ActionType act = nullptr);

  template <
    typename MsgT,
    ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
  >
  void send(MsgT* msg, ActionType act = nullptr);
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_SEND_SENDABLE_H*/
