
#if !defined INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_H
#define INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_H

#include "vt/config.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/activefn/activefn.h"

#include <functional>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct Reducable : BaseProxyT {
  using ReduceIdxFuncType = std::function<bool(IndexT const&)>;

  Reducable() = default;
  Reducable(Reducable const&) = default;
  Reducable(Reducable&&) = default;
  explicit Reducable(VirtualProxyType const in_proxy);
  Reducable& operator=(Reducable const&) = default;

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduce(
    MsgT *const msg, EpochType const& epoch = no_epoch,
    TagType const& tag = no_tag,
    NodeType const& node = uninitialized_destination
  );

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduceExpr(
    MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch = no_epoch,
    TagType const& tag = no_tag,
    NodeType const& node = uninitialized_destination
  );

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduce(
    MsgT *const msg, EpochType const& epoch, TagType const& tag,
    IndexT const& idx
  );

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  EpochType reduceExpr(
    MsgT *const msg, ReduceIdxFuncType fn, EpochType const& epoch,
    TagType const& tag,
    IndexT const& idx
  );
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_REDUCABLE_REDUCABLE_H*/
