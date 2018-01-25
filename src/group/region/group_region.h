
#if !defined INCLUDED_GROUP_REGION_GROUP_REGION_H
#define INCLUDED_GROUP_REGION_GROUP_REGION_H

#include "config.h"
#include "group/group_common.h"

#include <vector>
#include <cstdlib>

namespace vt { namespace group { namespace region {

struct Region {
  using BoundType = NodeType;
  using SizeType = size_t;
  using RegionPtr = Region*;

  virtual SizeType getSize() const = 0;
  //virtual bool overlaps(RegionPtr region) = 0;
};

struct Range : Region {
  Range(
    BoundType const& in_lo, BoundType const& in_hi, BoundType const& stride = 1
  );
  Range(Range const&) = default;
  Range(Range&&) = default;
  Range& operator=(Range const&) = default;

  virtual SizeType getSize() const override;

private:
  BoundType const lo_ = uninitialized_destination;
  BoundType const hi_ = uninitialized_destination;
  BoundType const stride_ = 1;
};

struct List : Region {
  using ListType = std::vector<BoundType>;

  explicit List(ListType const& in_list);

  List(List const&) = default;
  List(List&&) = default;
  List& operator=(List const&) = default;

  virtual SizeType getSize() const override;

private:
  ListType const list_;
};

}}} /* end namespace vt::group::region */

#endif /*INCLUDED_GROUP_REGION_GROUP_REGION_H*/
