
#include "config.h"
#include "context/context.h"
#include "group/group_common.h"
#include "group/region/group_region.h"
#include "group/region/group_range.h"

#include <vector>
#include <algorithm>
#include <cassert>

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

/*virtual*/ bool Range::isList() const {
  return false;
}

/*virtual*/ Range::BoundType Range::head() const {
  return lo_;
}

/*virtual*/ Range::RegionUPtrType Range::tail() const {
  return std::make_unique<Range>(lo_ + stride_, hi_, stride_);
}

/*virtual*/ Range::SplitRegionType Range::split() const {
  auto const& size = getSize();
  auto const& span = (hi_ - lo_) / 2;
  assert(
    size >= 2 && "Size must be at least 2 to split"
  );
  auto r1 = std::make_unique<Range>(lo_, lo_+span, stride_);
  auto r2 = std::make_unique<Range>(lo_+span, hi_, stride_);
  return std::make_tuple(std::move(r1),std::move(r2));
}

/*virtual*/ Range::RegionUPtrType Range::copy() const {
  return std::make_unique<Range>(*this);
}

}}} /* end namespace vt::group::region */
