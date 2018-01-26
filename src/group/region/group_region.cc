
#include "config.h"
#include "context/context.h"
#include "group/group_common.h"
#include "group/region/group_region.h"

#include <cassert>
#include <vector>

namespace vt { namespace group { namespace region {

Range::Range(
  BoundType const& in_lo, BoundType const& in_hi, BoundType const& in_stride
) : lo_(in_lo), hi_(in_hi), stride_(in_stride)
{
  assert(
    in_stride >= 1 && "Stride must be 1 more more: negative strides not allowed"
  );
}

/*virtual*/ Range::SizeType Range::getSize() const {
  return (hi_ - lo_) / stride_;
}

/*virtual*/ void Range::sort() {
  // do nothing, it's already sorted
}

/*virtual*/ bool Range::contains(NodeType const& node) {
  // Range overlap: x1 <= y2 && y1 <= x2
  if (made_list_) {
    // List must be sorted
    return std::binary_search(list_.begin(), list_.end(), node);
  } else {
    return node >= lo_ && node < hi_ && node % stride_ == 0;
  }
}

/*virtual*/ Range::ListType const& Range::makeList() {
  if (!made_list_) {
    ListType list;
    for (BoundType i = lo_; i < hi_; i += stride_) {
      list.push_back(i);
    }
    list_ = std::move(list);
    made_list_ = true;
  }
  return list_;
}

Range::Range(Range const& in_other, BoundType in_remove_extent)
  : Range(
    in_other.lo_ + in_remove_extent * in_other.stride_,
    in_other.hi_,
    in_other.stride_
  )
{ }

List::List(ListType const& in_list) : list_(in_list) { }

/*virtual*/ List::SizeType List::getSize() const {
  debug_print(
    group, node,
    "List::getSize(): size=%lu\n", list_.size()
  );
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

List::List(BoundType* list, SizeType const& size, bool const& is_sorted) {
  ListType new_list;
  for (auto elm = 0; elm < size; elm++) {
    new_list.push_back(list[elm]);
  }
  list_ = std::move(new_list);
  is_sorted_ = is_sorted;
}

}}} /* end namespace vt::group::region */
