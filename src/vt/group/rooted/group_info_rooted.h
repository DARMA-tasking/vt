
#if !defined INCLUDED_GROUP_GROUP_INFO_ROOTED_H
#define INCLUDED_GROUP_GROUP_INFO_ROOTED_H

#include "config.h"
#include "group/group_common.h"
#include "group/base/group_info_base.h"
#include "group/region/group_region.h"
#include "group/region/group_range.h"
#include "group/region/group_list.h"
#include "group/region/group_shallow_list.h"
#include "group/msg/group_msg.h"

#include <memory>
#include <vector>
#include <cstdlib>

namespace vt { namespace group {

struct InfoRooted : virtual InfoBase {
  using RegionType = region::Region;
  using RegionPtrType = std::unique_ptr<RegionType>;
  using ListType = std::vector<RegionType::BoundType>;

  InfoRooted(
    bool const& in_is_remote, RegionPtrType in_region,
    RegionType::SizeType const& in_total_size
  );

protected:
  void setupRooted();

protected:
  bool is_forward_                   = false;
  bool this_node_included_           = false;
  NodeType forward_node_             = uninitialized_destination;
  RegionPtrType region_              = nullptr;
  RegionType::SizeType total_size_   = 0;
  TreePtrType default_spanning_tree_ = nullptr;
  bool is_remote_                    = false;
  ListType region_list_              = {};
  WaitCountType wait_count_          = 0;
  bool is_setup_                     = false;
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_INFO_ROOTED_H*/
