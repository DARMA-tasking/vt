
#if !defined INCLUDED_GROUP_REGION_GROUP_LIST_H
#define INCLUDED_GROUP_REGION_GROUP_LIST_H

namespace vt { namespace group { namespace region {

struct List : Region {
  explicit List(ListType const& in_list);
  explicit List(ListType&& in_list);
  List(List const& in_other, BoundType in_remove_extent);
  List(BoundType const* const list, SizeType const& size, bool const& is_sorted);

  List(List const&) = default;
  List(List&&) = default;
  List& operator=(List const&) = default;

  friend struct ShallowList;

  virtual SizeType getSize() const override;
  virtual void sort() override;
  virtual bool contains(NodeType const& node) override;
  virtual ListType const& makeList() override;
  virtual bool isList() const override;
  virtual BoundType head() const override;
  virtual RegionUPtrType tail() const override;
  virtual SplitRegionType split() const override;
  virtual RegionUPtrType copy() const override;

private:
  bool is_sorted_ = false;
  ListType list_;
};

}}} /* end namespace vt::group::region */

#endif /*INCLUDED_GROUP_REGION_GROUP_LIST_H*/
