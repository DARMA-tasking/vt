
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
  for (auto elm = in_remove_extent; elm < list.size(); elm++) {
    new_list.push_back(list[elm]);
  }
  list_ = std::move(new_list);
  is_sorted_ = in_other.is_sorted_;
}

List::List(
  BoundType const* const list, SizeType const& size, bool const& is_sorted
) {
  ListType new_list;
  for (auto elm = 0; elm < size; elm++) {
    new_list.push_back(list[elm]);
  }
  list_ = std::move(new_list);
  is_sorted_ = is_sorted;
}

/*virtual*/ bool List::isList() const {
  return true;
}

/*virtual*/ List::BoundType List::head() const {
  assert(
    list_.size() > 0 && "Must be non-zero length to invoke head()"
  );
  return list_[0];
}

/*virtual*/ List::RegionUPtrType List::tail() const {
  ListType list;
  for (int i = 1; i < getSize(); i++) {
    list.push_back(list_[i]);
  }
  return std::make_unique<List>(std::move(list));
}

/*virtual*/ List::SplitRegionType List::split() const {
  auto const& size = getSize();
  assert(
    size >= 2 && "Size must be at least 2 to split"
  );

  ListType l1, l2;
  for (int i = 0; i < size; i++) {
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
