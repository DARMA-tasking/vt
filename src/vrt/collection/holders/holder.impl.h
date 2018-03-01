
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/holders/holder.h"

#include <unordered_map>
#include <tuple>

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
bool Holder<IndexT>::exists(IndexT const& idx ) {
  auto& container = vc_container_;
  auto iter = container.find(idx);
  return iter != container.end();
}

template <typename IndexT>
void Holder<IndexT>::insert(IndexT const& idx, InnerHolder&& inner) {
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

template <typename IndexT>
typename Holder<IndexT>::InnerHolder& Holder<IndexT>::lookup(
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

template <typename IndexT>
typename Holder<IndexT>::VirtualPtrType Holder<IndexT>::remove(
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

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H*/
