
#if !defined INCLUDED_VRT_COLLECTION_SEND_SENDABLE_H
#define INCLUDED_VRT_COLLECTION_SEND_SENDABLE_H

#include "config.h"
#include "vrt/proxy/base_collection.h"
#include "vrt/collection/active/active_funcs.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Sendable : BaseCollectionProxy<ColT, IndexT> {
  Sendable() = default;
  Sendable(
    typename BaseCollectionProxy<ColT, IndexT>::ProxyType const& in_proxy,
    typename BaseCollectionProxy<ColT, IndexT>::ElementProxyType const& in_elm
  );

  template <typename SerializerT>
  void serialize(SerializerT& s);

  template <
    typename MsgT,
    ActiveColTypedFnType<MsgT, typename MsgT::CollectionType> *f
  >
  void send(MsgT* msg, ActionType act = nullptr);
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_SEND_SENDABLE_H*/
