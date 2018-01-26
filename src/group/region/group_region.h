
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
  using ListType = std::vector<BoundType>;

  virtual SizeType getSize() const = 0;
  virtual void sort() = 0;
  virtual bool contains(NodeType const& node) = 0;
  virtual ListType const& makeList() = 0;
  //virtual bool overlaps(RegionPtr region) = 0;
};

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

private:
  BoundType const lo_ = uninitialized_destination;
  BoundType const hi_ = uninitialized_destination;
  BoundType const stride_ = 1;
  bool made_list_ = false;
  ListType list_;
};

struct List : Region {
  explicit List(ListType const& in_list);
  List(List const& in_other, BoundType in_remove_extent);
  List(BoundType* list, SizeType const& size, bool const& is_sorted);

  List(List const&) = default;
  List(List&&) = default;
  List& operator=(List const&) = default;

  virtual SizeType getSize() const override;
  virtual void sort() override;
  virtual bool contains(NodeType const& node) override;
  virtual ListType const& makeList() override;

private:
  bool is_sorted_ = false;
  ListType list_;
};

}}} /* end namespace vt::group::region */

#endif /*INCLUDED_GROUP_REGION_GROUP_REGION_H*/
