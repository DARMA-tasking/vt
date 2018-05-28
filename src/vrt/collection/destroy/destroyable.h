
#if !defined INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_H
#define INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_H

#include "config.h"
#include "vrt/proxy/base_wrapper.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Destroyable : BaseEntireCollectionProxy<ColT, IndexT> {
  Destroyable() = default;
  Destroyable(VirtualProxyType const in_proxy);

  void destroy();
};

template <typename ColT, typename IndexT>
struct Broadcastable : Destroyable<ColT, IndexT> {
  Broadcastable() = default;
  Broadcastable(VirtualProxyType const in_proxy);

  template <
    typename MsgT,
    ActiveColTypedFnType<MsgT, typename MsgT::CollectionType> *f
  >
  void broadcast(MsgT* msg, ActionType act = nullptr) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_H*/
