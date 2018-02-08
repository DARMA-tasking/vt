
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/manager.fwd.h"
#include "vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vrt/collection/types/headers.h"

#include <unordered_map>
#include <tuple>

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Holder {
  template <typename T, typename U>
  using ContType = std::unordered_map<T, U>;
  using CollectionType = Collection<IndexT>;
  using VirtualPtrType = std::unique_ptr<CollectionType>;
  using LookupElementType = std::tuple<VirtualProxyType, IndexT>;

  struct InnerHolder {
    VirtualPtrType vc_ptr_;
    HandlerType map_fn = uninitialized_handler;
    IndexT max_idx;

    InnerHolder(
      VirtualPtrType in_vc_ptr_, HandlerType const& in_han, IndexT const& idx
    ) : vc_ptr_(std::move(in_vc_ptr_)), map_fn(in_han), max_idx(idx)
    { }

    typename VirtualPtrType::pointer getCollection() {
      assert(vc_ptr_ != nullptr and "Must be valid pointer");
      return vc_ptr_.get();
    }
  };

  using TypedIndexContainer = ContType<LookupElementType, InnerHolder>;

  static bool exists(VirtualProxyType const& proxy, IndexT const& idx);
  static InnerHolder& lookup(VirtualProxyType const& proxy, IndexT const& idx);
  static void insert(
    VirtualProxyType const& proxy, IndexT const& idx, InnerHolder&& inner
  );

  friend struct CollectionManager;

private:
  static TypedIndexContainer vc_container_;
};

template <typename IndexT>
typename Holder<IndexT>::TypedIndexContainer Holder<IndexT>::vc_container_;

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/holders/holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_H*/
