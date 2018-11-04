
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CHILD_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CHILD_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_types.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

struct HierLBChild : HierLBTypes {
  HierLBChild() = default;
  HierLBChild(HierLBChild const&) = default;
  HierLBChild(HierLBChild&&) = default;

  bool is_live = false;
  double cur_load = 0.0;
  int32_t node_size = 0;
  NodeType node = uninitialized_destination;
  bool final_child = false;
  std::size_t recs_size = 0;
  ObjSampleType recs;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CHILD_H*/
