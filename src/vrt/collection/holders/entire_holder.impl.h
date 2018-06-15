
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_IMPL_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_IMPL_H

#include "config.h"
#include "vrt/collection/holders/entire_holder.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::destroyAllLive() {
  for (auto&& elm : live_collections_) {
    elm->destroy();
  }
  live_collections_.clear();
}

template <typename always_void_>
/*static*/ bool UniversalIndexHolder<always_void_>::readyNextPhase() {
  return num_collections_phase_ == getNumCollections();
}

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::makeCollectionReady() {
  num_collections_phase_++;
}

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::resetPhase() {
  num_collections_phase_ = 0;
}

template <typename always_void_>
/*static*/ std::size_t UniversalIndexHolder<always_void_>::getNumCollections() {
  return live_collections_.size();
}

template <typename always_void_>
/*static*/ std::unordered_set<std::shared_ptr<BaseHolder>>
UniversalIndexHolder<always_void_>::live_collections_;

template <typename always_void_>
/*static*/ std::size_t
UniversalIndexHolder<always_void_>::num_collections_phase_ = 0;

template <typename ColT, typename IndexT>
/*static*/ void EntireHolder<ColT, IndexT>::insert(
  VirtualProxyType const& proxy, InnerHolderPtr ptr
) {
  proxy_container_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(proxy),
    std::forward_as_tuple(ptr)
  );
  UniversalIndexHolder<>::live_collections_.insert(ptr);
}

template <typename ColT, typename IndexT>
/*static*/ typename EntireHolder<ColT, IndexT>::ProxyContainerType
EntireHolder<ColT, IndexT>::proxy_container_;

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_IMPL_H*/
