
#include "config.h"
#include "group/group_common.h"
#include "group/region/group_region.h"

namespace vt { namespace group { namespace region {

Range::Range(
  BoundType const& in_lo, BoundType const& in_hi, BoundType const& in_stride
) : lo_(in_lo), hi_(in_hi), stride_(in_stride)
{ }

/*virtual*/ Range::SizeType Range::getSize() const {
  return (hi_ - lo_) / stride_;
}

List::List(ListType const& in_list) : list_(in_list) { }

/*virtual*/ List::SizeType List::getSize() const {
  return list_.size();
}

}}} /* end namespace vt::group::region */
