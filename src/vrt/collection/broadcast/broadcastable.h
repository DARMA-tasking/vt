
#if !defined INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H
#define INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H

#include "config.h"
#include "vrt/collection/destroy/destroyable.h"
#include "vrt/proxy/base_wrapper.h"
#include "activefn/activefn.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Broadcastable : Destroyable<ColT, IndexT> {
  using ReduceIdxFuncType = std::function<bool(IndexT const&)>;

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

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduce(
    MsgT *const msg, EpochType const& epoch = no_epoch,
    TagType const& tag = no_tag
  );

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduceExpr(
    MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch = no_epoch,
    TagType const& tag = no_tag
  );

  void finishedInserting(ActionType action = nullptr) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_BROADCAST_BROADCASTABLE_H*/
