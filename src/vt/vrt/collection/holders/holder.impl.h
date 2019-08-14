/*
//@HEADER
// ************************************************************************
//
//                          holder.impl.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/holders/holder.h"

#include <unordered_map>
#include <tuple>
#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
bool Holder<ColT, IndexT>::exists(IndexT const& idx ) {
  auto& container = vc_container_;
  auto iter = container.find(idx);
  return iter != container.end() and not iter->second.erased_;
}

template <typename ColT, typename IndexT>
void Holder<ColT, IndexT>::insert(IndexT const& idx, InnerHolder&& inner) {
  vtAssert(!is_destroyed_, "Must not be destroyed to insert a new element");
  vtAssert(!exists(idx), "Should not exist to insert element");

  auto const& lookup = idx;
  auto& container = vc_container_;

  auto iter = container.find(lookup);
  if (iter != container.end()) {
    vtAssert(iter->second.erased_, "Must be in erased state");
    num_erased_not_removed_--;
    container.erase(iter);
  }

  /*
   * This assertion no longer valid due to delayed erasure. In fact, the inner
   * holder VC pointer may be nullptr but set to erased. The move should deal
   * with this problem.
   *
   *   auto iter = container.find(lookup);
   *   vtAssert(
   *     iter == container.end(),
   *     "Entry must not exist in holder when inserting"
   *   );
   */
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
  vtAssert(
    iter != container.end(), "Entry must exist in holder when searching"
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
  vtAssert(
    iter != container.end(), "Entry must exist in holder when removing entry"
  );
  auto owned_ptr = std::move(iter->second.vc_ptr_);
  vtAssert(iter->second.erased_ == false, "Must not be erased already");
  iter->second.erased_ = true;
  num_erased_not_removed_++;
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
void Holder<ColT, IndexT>::cleanupExists() {
  auto& container = vc_container_;
  for (auto iter = container.begin(); iter != container.end(); ) {
    if (iter->second.erased_) {
      num_erased_not_removed_--;
      iter = container.erase(iter);
    } else {
      ++iter;
    }
  }
}

template <typename ColT, typename IndexT>
bool Holder<ColT, IndexT>::foreach(FuncApplyType fn) {
  static uint64_t num_reentrant = 0;

  num_reentrant++;
  auto& container = vc_container_;
  for (auto& elm : container) {
    if (!elm.second.erased_) {
      auto const& idx = elm.first;
      auto const& holder = elm.second;
      auto const col_ptr = holder.getCollection();
      fn(idx,col_ptr);
    }
  }
  num_reentrant--;

  if (num_reentrant == 0) {
    cleanupExists();
  }
  return true;
}

template <typename ColT, typename IndexT>
typename Holder<ColT,IndexT>::TypedIndexContainer::size_type
Holder<ColT,IndexT>::numElements() const {
  return vc_container_.size() - num_erased_not_removed_;
}

template <typename ColT, typename IndexT>
typename Holder<ColT,IndexT>::TypedIndexContainer::size_type
Holder<ColT,IndexT>::numElementsExpr(FuncExprType fn) const {
  typename Holder<ColT,IndexT>::TypedIndexContainer::size_type num_in = 0;
  for (auto&& elm : vc_container_) {
    num_in += fn(elm.first);
  }
  return num_in;
}

template <typename ColT, typename IndexT>
void Holder<ColT, IndexT>::addLBCont(IndexT const& idx, LBContFnType fn) {
  using MappedType = typename TypedLBContainer::mapped_type;
  auto iter = vc_lb_continuation_.find(idx);
  if (iter == vc_lb_continuation_.end()) {
    vc_lb_continuation_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(idx),
      std::forward_as_tuple(MappedType{})
    );
    iter = vc_lb_continuation_.find(idx);
  }
  vtAssert(iter != vc_lb_continuation_.end(), "Key must exist in map");
  iter->second.push_back(fn);
}

template <typename ColT, typename IndexT>
void Holder<ColT, IndexT>::runLBCont(IndexT const& idx) {
  auto iter = vc_lb_continuation_.find(idx);
  if (iter != vc_lb_continuation_.end()) {
    for (auto&& elm : iter->second) {
      elm();
    }
    vc_lb_continuation_.erase(iter);
  }
}

template <typename ColT, typename IndexT>
void Holder<ColT, IndexT>::runLBCont() {
  for (auto&& idxc : vc_lb_continuation_) {
    for (auto&& elm : idxc.second) {
      elm();
    }
  }
  vc_lb_continuation_.clear();
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H*/
