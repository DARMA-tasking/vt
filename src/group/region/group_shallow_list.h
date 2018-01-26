
#if !defined INCLUDED_GROUP_REGION_GROUP_SHALLOW_LIST_H
#define INCLUDED_GROUP_REGION_GROUP_SHALLOW_LIST_H

#include "config.h"
#include "context/context.h"
#include "group/group_common.h"
#include "group/region/group_region.h"

namespace vt { namespace group { namespace region {

struct ShallowList : Region {
  ShallowList(BoundType const* in_bound, SizeType const& size);
  explicit ShallowList(List const& in_list);
  explicit ShallowList(ListType const& in_list);

  ShallowList(ShallowList const&) = default;
  ShallowList(ShallowList&&) = default;
  ShallowList& operator=(ShallowList const&) = default;

  virtual SizeType getSize() const override;
  virtual void sort() override;
  virtual bool contains(NodeType const& node) override;
  virtual ListType const& makeList() override;
  virtual bool isList() const override;
  virtual BoundType head() const override;
  virtual RegionUPtrType tail() const override;
  virtual SplitRegionType split() const override;
  virtual BoundType const* getBound() const;
  virtual RegionUPtrType copy() const override;

private:
  SizeType size_ = 0;
  BoundType const* bound_ = nullptr;
};

}}} /* end namespace vt::group::region */

#endif /*INCLUDED_GROUP_REGION_GROUP_SHALLOW_LIST_H*/
