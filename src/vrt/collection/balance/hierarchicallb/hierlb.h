
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H

#include "config.h"
#include "vrt/collection/balance/hierarchicallb/hierlb.fwd.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_types.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_child.h"

#include <unordered_map>
#include <memory>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct HierarchicalLB : HierLBTypes {
  using ChildPtrType = std::unique_ptr<HierLBChild>;
  using ChildMapType = std::unordered_map<NodeType,ChildPtrType>;

  HierarchicalLB() = default;

  void setupTree();
  void calcLoadOver();

private:
  bool tree_setup = false;
  NodeType parent = uninitialized_destination;
  NodeType bottomParent = uninitialized_destination;
  ChildMapType children, live_children;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H*/
