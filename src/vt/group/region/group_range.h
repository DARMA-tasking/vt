
#if !defined INCLUDED_GROUP_REGION_GROUP_RANGE_H
#define INCLUDED_GROUP_REGION_GROUP_RANGE_H

#include "config.h"
#include "context/context.h"
#include "group/group_common.h"
#include "group/region/group_region.h"

namespace vt { namespace group { namespace region {

struct Range : Region {
  Range(
    BoundType const& in_lo, BoundType const& in_hi, BoundType const& stride = 1
  );
  Range(Range const& in_other, BoundType in_remove_extent);

  Range(Range const&) = default;
  Range(Range&&) = default;
  Range& operator=(Range const&) = default;

  virtual SizeType getSize() const override;
  virtual void sort() override;
  virtual bool contains(NodeType const& node) override;
  virtual ListType const& makeList() override;
  virtual bool isList() const override;
  virtual BoundType head() const override;
  virtual RegionUPtrType tail() const override;
  virtual SplitRegionType split() const override;
  virtual RegionUPtrType copy() const override;
  virtual void splitN(int nsplits, ApplyFnType apply) const override;

private:
  BoundType const lo_ = uninitialized_destination;
  BoundType const hi_ = uninitialized_destination;
  BoundType const stride_ = 1;
  bool made_list_ = false;
  ListType list_;
};

}}} /* end namespace vt::group::region */

#endif /*INCLUDED_GROUP_REGION_GROUP_RANGE_H*/
