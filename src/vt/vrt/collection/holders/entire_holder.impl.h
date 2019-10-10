/*
//@HEADER
// *****************************************************************************
//
//                             entire_holder.impl.h
//                           DARMA Toolkit v. 1.0.0
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

#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_IMPL_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_IMPL_H

#include "vt/config.h"
#include "vt/epoch/epoch_headers.h"
#include "vt/vrt/collection/holders/entire_holder.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::destroyAllLive() {
  for (auto&& elm : live_collections_) {
    elm.second->destroy();
  }
  live_collections_.clear();
}

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::runLB(PhaseType phase) {
  for (auto&& elm : live_collections_) {
    auto base_holder = elm.second;
    base_holder->runLB(phase);
  }
}

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::destroyCollection(
  VirtualProxyType const proxy
) {
  auto iter = live_collections_.find(proxy);
  if (iter != live_collections_.end()) {
    live_collections_.erase(iter);
  }
}

template <typename always_void_>
/*static*/ bool UniversalIndexHolder<always_void_>::readyNextPhase() {
  auto const ready_coll = getNumReadyCollections();
  auto const total_coll = getNumCollections();
  return ready_coll == total_coll;
}

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::makeCollectionReady(
  VirtualProxyType const proxy
) {
  ready_collections_.insert(proxy);
}

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::resetPhase() {
  ready_collections_.clear();
}

template <typename always_void_>
/*static*/ std::size_t
UniversalIndexHolder<always_void_>::getNumCollections() {
  return live_collections_.size();
}

template <typename always_void_>
/*static*/ std::size_t
UniversalIndexHolder<always_void_>::getNumReadyCollections() {
  return ready_collections_.size();
}

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::insertMap(
  VirtualProxyType const proxy, HandlerType const& han,
  EpochType const& insert_epoch
) {
  live_collections_map_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(proxy),
    std::forward_as_tuple(han)
  );
  insert_epoch_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(proxy),
    std::forward_as_tuple(insert_epoch)
  );
}

template <typename always_void_>
/*static*/ void UniversalIndexHolder<always_void_>::insertSetEpoch(
  VirtualProxyType const proxy, EpochType const& insert_epoch
) {
  auto iter = insert_epoch_.find(proxy);
  vtAssert(iter != insert_epoch_.end(), "Proxy must exist in insert epoch");
  iter->second = insert_epoch;
}

template <typename always_void_>
/*static*/ EpochType UniversalIndexHolder<always_void_>::insertGetEpoch(
  VirtualProxyType const proxy
) {
  auto iter = insert_epoch_.find(proxy);
  vtAssert(iter != insert_epoch_.end(), "Proxy must exist in insert epoch");
  return iter->second;
}

template <typename always_void_>
/*static*/ HandlerType UniversalIndexHolder<always_void_>::getMap(
  VirtualProxyType const proxy
) {
  auto map_iter = live_collections_map_.find(proxy);
  vtAssert(map_iter != live_collections_map_.end(), "Map must exist");
  return map_iter->second;
}

template <typename always_void_>
/*static*/ std::unordered_map<VirtualProxyType,std::shared_ptr<BaseHolder>>
UniversalIndexHolder<always_void_>::live_collections_;

template <typename always_void_>
/*static*/ std::unordered_map<VirtualProxyType,HandlerType>
UniversalIndexHolder<always_void_>::live_collections_map_;

template <typename always_void_>
/*static*/ std::unordered_set<VirtualProxyType>
UniversalIndexHolder<always_void_>::ready_collections_ = {};

template <typename always_void_>
/*static*/ std::unordered_map<VirtualProxyType,EpochType>
UniversalIndexHolder<always_void_>::insert_epoch_ = {};

template <typename ColT, typename IndexT>
/*static*/ void EntireHolder<ColT, IndexT>::insert(
  VirtualProxyType const& proxy, InnerHolderPtr ptr
) {
  proxy_container_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(proxy),
    std::forward_as_tuple(ptr)
  );
  UniversalIndexHolder<>::live_collections_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(proxy),
    std::forward_as_tuple(ptr)
  );
}

template <typename ColT, typename IndexT>
/*static*/ void EntireHolder<ColT, IndexT>::remove(
  VirtualProxyType const& proxy
) {
  auto iter = proxy_container_.find(proxy);
  if (iter != proxy_container_.end()) {
    proxy_container_.erase(iter);
  }
}

template <typename ColT, typename IndexT>
/*static*/ typename EntireHolder<ColT, IndexT>::ProxyContainerType
EntireHolder<ColT, IndexT>::proxy_container_;

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_IMPL_H*/
