/*
//@HEADER
// ************************************************************************
//
//                          group_list.cc
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

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/group/group_common.h"
#include "vt/group/region/group_region.h"
#include "vt/group/region/group_list.h"

#include <vector>
#include <algorithm>
#include <cassert>

namespace vt { namespace group { namespace region {

List::List(ListType const& in_list) : list_(in_list) { }

List::List(ListType&& in_list) : list_(std::move(in_list)) { }

/*virtual*/ List::SizeType List::getSize() const {
  return list_.size();
}

/*virtual*/ void List::sort() {
  if (!is_sorted_) {
    std::sort(list_.begin(), list_.end());
    is_sorted_ = true;
  }
}

/*virtual*/ bool List::contains(NodeType const& node) {
  if (is_sorted_) {
    return std::binary_search(list_.begin(), list_.end(), node);
  } else {
    for (auto&& elm : list_) {
      if (elm == node) {
        return true;
      }
    }
  }
  return false;
}

/*virtual*/ List::ListType const& List::makeList() {
  return list_;
}

List::List(List const& in_other, BoundType in_remove_extent) {
  auto const& list = in_other.list_;
  ListType new_list;
  auto old_size = list.size();
  for (decltype(old_size) elm = in_remove_extent; elm < old_size; elm++) {
    new_list.push_back(list[elm]);
  }
  list_ = std::move(new_list);
  is_sorted_ = in_other.is_sorted_;
}

List::List(
  BoundType const* const list, SizeType const& size, bool const& is_sorted
) {
  ListType new_list;
  for (SizeType elm = 0; elm < size; elm++) {
    new_list.push_back(list[elm]);
  }
  list_ = std::move(new_list);
  is_sorted_ = is_sorted;
}

/*virtual*/ bool List::isList() const {
  return true;
}

/*virtual*/ List::BoundType List::head() const {
  vtAssert(
    list_.size() > 0, "Must be non-zero length to invoke head()"
  );
  return list_[0];
}

/*virtual*/ List::RegionUPtrType List::tail() const {
  ListType list;
  auto size = getSize();
  for (decltype(size) i = 1; i < size; i++) {
    list.push_back(list_[i]);
  }
  return std::make_unique<List>(std::move(list));
}

/*virtual*/ List::SplitRegionType List::split() const {
  auto const& size = getSize();
  vtAssert(
    size >= 2, "Size must be at least 2 to split"
  );

  ListType l1, l2;
  for (size_t i = 0; i < size; i++) {
    if (i < size/2) {
      l1.push_back(list_[i]);
    } else {
      l2.push_back(list_[i]);
    }
  }

  auto r1 = std::make_unique<List>(std::move(l1));
  auto r2 = std::make_unique<List>(std::move(l2));
  return std::make_tuple(std::move(r1),std::move(r2));
}

/*virtual*/ List::RegionUPtrType List::copy() const {
  auto list = std::make_unique<List>(*this);
  return std::move(list);
}

/*virtual*/ void List::splitN(int nsplits, ApplyFnType apply) const {
  auto const& size = static_cast<int>(getSize());
  auto const& num_splits = std::min(nsplits, size);
  int cur_idx = 0;
  for (auto split = 0; split < num_splits; split++) {
    auto const& child_size = size / num_splits;
    auto const& cur_max = split == num_splits - 1 ?
      size : std::min(size, cur_idx + child_size);
    ListType list;
    for (int i = cur_idx; i < cur_max; i++) {
      list.push_back(list_[i]);
    }
    auto r1 = std::make_unique<List>(std::move(list));
    apply(std::move(r1));
    cur_idx = cur_max;
  }
}

}}} /* end namespace vt::group::region */
