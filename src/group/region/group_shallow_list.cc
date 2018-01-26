
#include "config.h"
#include "group/region/group_region.h"
#include "group/region/group_shallow_list.h"
#include "group/region/group_list.h"

namespace vt { namespace group { namespace region {

ShallowList::ShallowList(BoundType const* in_bound, SizeType const& in_size)
  : bound_(in_bound), size_(in_size)
{ }

ShallowList::ShallowList(List const& in_list)
  : ShallowList(in_list.list_)
{ }

ShallowList::ShallowList(ListType const& in_list)
  : ShallowList(&in_list[0], in_list.size())
{ }

/*virtual*/ ShallowList::SizeType ShallowList::getSize() const {
  return size_;
}

/*virtual*/ void ShallowList::sort() {
  assert(0 && "Can not be sorted");
}

/*virtual*/ bool ShallowList::contains(NodeType const& node) {
  for (int i = 0; i < size_; i++) {
    if (bound_[i] == node) {
      return true;
    }
  }
  return false;
}

/*virtual*/ ShallowList::ListType const& ShallowList::makeList() {
  assert(0 && "Can not be implemented for ShallowList");
  // typename ShallowList::ListType list;
  // for (auto i = 0; i < size_; i++) {
  //   list.push_back(bound_[i]);
  // }
  // return list;
}

/*virtual*/ bool ShallowList::isList() const {
  return true;
}

/*virtual*/ ShallowList::BoundType ShallowList::head() const {
  return *bound_;
}

/*virtual*/ ShallowList::RegionUPtrType ShallowList::tail() const {
  return std::make_unique<ShallowList>(bound_+1, size_-1);
}

/*virtual*/ ShallowList::SplitRegionType ShallowList::split() const {
  assert(
    size_ >= 2 && "Size must be at least 2 to split"
  );
  auto const& c1_size = size_ / 2;
  auto const& c2_size = size_ - c1_size;
  auto r1 = std::make_unique<ShallowList>(bound_, c1_size);
  auto r2 = std::make_unique<ShallowList>(bound_ + c1_size, c2_size);
  return std::make_tuple(std::move(r1),std::move(r2));
}

/*virtual*/ ShallowList::BoundType const* ShallowList::getBound() const {
  return bound_;
}

/*virtual*/ ShallowList::RegionUPtrType ShallowList::copy() const {
  return std::make_unique<List>(bound_, size_, true);
}

}}} /* end namespace vt::group::region */
