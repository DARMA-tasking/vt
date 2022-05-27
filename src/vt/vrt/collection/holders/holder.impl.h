/*
//@HEADER
// *****************************************************************************
//
//                                holder.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/holders/holder.h"

#include <unordered_map>
#include <tuple>
#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
bool Holder<IndexT>::exists(IndexT const& idx ) {
  auto& container = vc_container_;
  auto iter = container.find(idx);
  return iter != container.end() and not iter->second.erased_;
}

template <typename IndexT>
void Holder<IndexT>::insert(IndexT const& idx, InnerHolder&& inner) {
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

template <typename IndexT>
typename Holder<IndexT>::InnerHolder& Holder<IndexT>::lookup(
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

template <typename IndexT>
typename Holder<IndexT>::VirtualPtrType Holder<IndexT>::remove(
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
  return owned_ptr;
}

template <typename IndexT>
void Holder<IndexT>::destroyAll() {
  if (!is_destroyed_) {
    vc_container_.clear();
    is_destroyed_ = true;
  }
}

template <typename IndexT>
bool Holder<IndexT>::isDestroyed() const {
  return is_destroyed_;
}

template <typename IndexT>
void Holder<IndexT>::cleanupExists() {
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

template <typename IndexT>
void Holder<IndexT>::foreach(FuncApplyType fn) {
  static uint64_t num_reentrant = 0;

  num_reentrant++;
  auto& container = vc_container_;
  for (auto& elm : container) {
    if (!elm.second.erased_) {
      auto const& idx = elm.first;
      auto const& holder = elm.second;
      auto const col_ptr = holder.getRawPtr();
      fn(idx, col_ptr);
    }
  }
  num_reentrant--;

  if (num_reentrant == 0) {
    cleanupExists();
  }
}

template <typename IndexT>
typename Holder<IndexT>::TypedIndexContainer::size_type
Holder<IndexT>::numElements() const {
  return vc_container_.size() - num_erased_not_removed_;
}

template <typename IndexT>
typename Holder<IndexT>::TypedIndexContainer::size_type
Holder<IndexT>::numElementsExpr(FuncExprType fn) const {
  typename Holder<IndexT>::TypedIndexContainer::size_type num_in = 0;
  for (auto&& elm : vc_container_) {
    num_in += fn(elm.first);
  }
  return num_in;
}

template <typename IndexT>
int Holder<IndexT>::addListener(listener::ListenFnType<IndexT> fn) {
  event_listeners_.push_back(fn);
  return event_listeners_.size() - 1;
}

template <typename IndexT>
void Holder<IndexT>::removeListener(int element) {
  vtAssert(
    event_listeners_.size() > static_cast<size_t>(element),
    "Listener must exist"
  );
  event_listeners_[element] = nullptr;
}

template <typename IndexT>
void Holder<IndexT>::applyListeners(
  listener::ElementEventEnum event, IndexT const& idx, NodeType home_node
) {
  for (auto&& l : event_listeners_) {
    if (l) {
      l(event, idx, home_node);
    }
  }
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_HOLDERS_HOLDER_IMPL_H*/
