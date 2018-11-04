
#if !defined INCLUDED_GROUP_REGION_GROUP_REGION_H
#define INCLUDED_GROUP_REGION_GROUP_REGION_H

#include "vt/config.h"
#include "vt/group/group_common.h"

#include <vector>
#include <cstdlib>
#include <memory>
#include <tuple>
#include <functional>

namespace vt { namespace group { namespace region {

struct Region {
  using BoundType = NodeType;
  using SizeType = size_t;
  using RegionPtr = Region*;
  using RegionUPtrType = std::unique_ptr<Region>;
  using SplitRegionType = std::tuple<RegionUPtrType, RegionUPtrType>;
  using ListType = std::vector<BoundType>;
  using ApplyFnType = std::function<void(RegionUPtrType)>;

  virtual ~Region(){}
  virtual SizeType getSize() const = 0;
  virtual void sort() = 0;
  virtual bool contains(NodeType const& node) = 0;
  virtual bool isList() const = 0;
  virtual ListType const& makeList() = 0;
  virtual RegionUPtrType copy() const = 0;
  virtual BoundType head() const = 0;
  virtual RegionUPtrType tail() const = 0;
  virtual SplitRegionType split() const = 0;
  virtual void splitN(int nsplits, ApplyFnType apply) const = 0;
};

struct List;
struct Range;
struct ShallowList;

}}} /* end namespace vt::group::region */

#endif /*INCLUDED_GROUP_REGION_GROUP_REGION_H*/
