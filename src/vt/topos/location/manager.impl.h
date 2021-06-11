/*
//@HEADER
// *****************************************************************************
//
//                                manager.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H
#define INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H

#include "vt/config.h"
#include "vt/topos/location/manager.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/location.h"

#include <memory>
#include <cassert>
#include <unordered_map>
#include <functional>

namespace vt { namespace location {

template <typename ColT, typename IndexT>
void LocationManager::insertCollectionLM(VirtualProxyType const& proxy) {
  using LocType = VrtColl<ColT, IndexT>;
  auto loc_man_typed = new LocType(
    collection_lm_tag_t{}, static_cast<LocInstType>(proxy)
  );
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
}

template <typename ColT, typename IndexT>
LocationManager::VrtColl<ColT, IndexT>*
LocationManager::getCollectionLM(VirtualProxyType const& proxy) {
  using LocType = VrtColl<ColT, IndexT>;

  auto loc_iter = collectionLoc.find(proxy);
  auto const& found = loc_iter != collectionLoc.end();

  vt_debug_print(
    normal, location,
    "getCollectionLM: proxy={}, found={}\n",
    proxy, print_bool(found)
  );

  if (!found) {
    LocationManager::insertCollectionLM<ColT, IndexT>(proxy);
    loc_iter = collectionLoc.find(proxy);
  } else if (!found) {
    return nullptr;
  }
  vtAssert(
    loc_iter != collectionLoc.end() && loc_iter->second != nullptr,
    "Location manager must exist now for this collection"
  );
  auto manager = loc_iter->second.get();
  auto const& typed_loc_ptr = static_cast<LocType*>(manager);
  return typed_loc_ptr;
}

template <typename LocType>
/*static*/ void LocationManager::applyInstance(
  LocInstType const inst, ActionLocInstType<LocType> action
) {
  auto inst_iter = loc_insts.find(inst);
  if (inst_iter == loc_insts.end()) {
    auto pending_iter = details::PendingInst<LocType>::pending_inst_.find(inst);
    if (pending_iter == details::PendingInst<LocType>::pending_inst_.end()) {
      details::PendingInst<LocType>::pending_inst_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(inst),
        std::forward_as_tuple(PendingContainerType<LocType>{action})
      );
    } else {
      pending_iter->second.push_back(action);
    }
  } else {
    auto const& inst_ret = LocationManager::getInstance(inst);
    action(static_cast<LocType*>(inst_ret));
  }
}

template <typename LocType>
/*static*/ void LocationManager::insertInstance(LocInstType const inst, LocType* ptr) {
  loc_insts.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(inst),
    std::forward_as_tuple(static_cast<LocCoordPtrType>(ptr))
  );

  auto iter = details::PendingInst<LocType>::pending_inst_.find(inst);
  if (iter != details::PendingInst<LocType>::pending_inst_.end()) {
    for (auto&& elm : iter->second) {
      elm(ptr);
    }
    details::PendingInst<LocType>::pending_inst_.erase(iter);
  }
}


namespace details {
template <typename LocType>
/*static*/ std::unordered_map<
  LocInstType, LocationManager::PendingContainerType<LocType>
> PendingInst<LocType>::pending_inst_;
}

}} /* end namespace vt::location */

#endif /*INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H*/
