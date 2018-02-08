
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/holders/holder.h"

#include <unordered_map>
#include <tuple>

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
/*static*/ bool Holder<IndexT>::exists(
  VirtualProxyType const& proxy, IndexT const& idx
) {
  auto& container = Holder<IndexT>::vc_container_;
  auto iter = container.find(std::make_tuple(proxy,idx));
  return iter != container.end();
}

template <typename IndexT>
/*static*/ void Holder<IndexT>::insert(
  VirtualProxyType const& proxy, IndexT const& idx, InnerHolder&& inner
) {
  auto const& lookup = std::make_tuple(proxy,idx);
  auto& container = Holder<IndexT>::vc_container_;
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
/*static*/ typename Holder<IndexT>::InnerHolder&
Holder<IndexT>::lookup(VirtualProxyType const& proxy, IndexT const& idx) {
  auto const& lookup = std::make_tuple(proxy,idx);
  auto& container = Holder<IndexT>::vc_container_;
  auto iter = container.find(lookup);
  assert(
    iter != container.end() && "Entry must exist in holder when searching"
  );
  return iter->second;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H*/
