
#if !defined INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H
#define INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H

#include "config.h"
#include "topos/location/manager.h"
#include "topos/location/location_common.h"
#include "topos/location/location.h"

#include <memory>
#include <cassert>
#include <unordered_map>
#include <functional>

namespace vt { namespace location {

template <typename IndexT>
LocationManager::VrtColl<IndexT>*
LocationManager::getCollectionLM(VirtualProxyType const& proxy) {
  using LocType = VrtColl<IndexT>;

  auto loc_iter = collectionLoc.find(proxy);
  if (loc_iter == collectionLoc.end()) {
    auto loc_man_typed = new LocType();
    auto loc_ptr = std::unique_ptr<LocErasureType, LocDeleterType>(
      static_cast<LocErasureType*>(loc_man_typed),
      [](LocErasureType* elm) {
        auto const& typed_elm = static_cast<LocType*>(elm);
        delete typed_elm;
      }
    );
    collectionLoc.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(proxy),
      std::forward_as_tuple(std::move(loc_ptr))
    );
    loc_iter = collectionLoc.find(proxy);
  }
  assert(
    (loc_iter != collectionLoc.end() && loc_iter->second != nullptr) &&
    "Location manager must exist now for this collection"
  );
  auto manager = loc_iter->second.get();
  auto const& typed_loc_ptr = static_cast<LocType*>(manager);
  return typed_loc_ptr;
}

}} /* end namespace vt::location */

#endif /*INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H*/
