
#if !defined INCLUDED_VRT_COLLECTION_SEND_SENDABLE_H
#define INCLUDED_VRT_COLLECTION_SEND_SENDABLE_H

#include "config.h"
#include "vrt/proxy/base_collection.h"
#include "vrt/collection/active/active_funcs.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Sendable : BaseCollectionProxy<IndexT> {
  Sendable() = default;
  Sendable(
    typename BaseCollectionProxy<IndexT>::ProxyType const& in_proxy,
    typename BaseCollectionProxy<IndexT>::ElementProxyType const& in_elm_proxy
  );

  template <typename SerializerT>
  void serialize(SerializerT& s);

  template <typename ColT, typename MsgT, ActiveColTypedFnType<MsgT, ColT> *f>
  void send(MsgT* msg, ActionType act = nullptr);
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_SEND_SENDABLE_H*/
