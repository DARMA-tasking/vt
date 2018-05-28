
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/holders/holder.h"

#include <unordered_map>
#include <tuple>
#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
bool Holder<ColT, IndexT>::exists(IndexT const& idx ) {
  auto& container = vc_container_;
  auto iter = container.find(idx);
  return iter != container.end();
}

template <typename ColT, typename IndexT>
void Holder<ColT, IndexT>::insert(IndexT const& idx, InnerHolder&& inner) {
  assert(!is_destroyed_ && "Must not be destroyed to insert a new element");

  auto const& lookup = idx;
  auto& container = vc_container_;
  auto iter = container.find(lookup);
  assert(
    iter == container.end() && "Entry must not exist in holder when inserting"
  );
  container.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(lookup),
    std::forward_as_tuple(std::move(inner))
  );
}

template <typename ColT, typename IndexT>
typename Holder<ColT, IndexT>::InnerHolder& Holder<ColT, IndexT>::lookup(
  IndexT const& idx
) {
  auto const& lookup = idx;
  auto& container = vc_container_;
  auto iter = container.find(lookup);
  assert(
    iter != container.end() && "Entry must exist in holder when searching"
  );
  return iter->second;
}

template <typename ColT, typename IndexT>
typename Holder<ColT, IndexT>::VirtualPtrType Holder<ColT, IndexT>::remove(
  IndexT const& idx
) {
  auto const& lookup = idx;
  auto& container = vc_container_;
  auto iter = container.find(lookup);
  assert(
    iter != container.end() && "Entry must exist in holder when removing entry"
  );
  auto owned_ptr = std::move(iter->second.vc_ptr_);
  container.erase(iter);
  return std::move(owned_ptr);
}

template <typename ColT, typename IndexT>
void Holder<ColT, IndexT>::destroyAll() {
  if (!is_destroyed_) {
    vc_container_.clear();
    is_destroyed_ = true;
  }
}

template <typename ColT, typename IndexT>
bool Holder<ColT, IndexT>::isDestroyed() const {
  return is_destroyed_;
}

template <typename ColT, typename IndexT>
bool Holder<ColT, IndexT>::foreach(FuncApplyType fn) {
  auto& container = vc_container_;
  for (auto&& elm : container) {
    auto const& idx = elm.first;
    auto const& holder = elm.second;
    auto const col_ptr = holder.getCollection();
    fn(idx,col_ptr);
  }
  return true;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H*/
