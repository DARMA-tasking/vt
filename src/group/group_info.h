
#if !defined INCLUDED_GROUP_GROUP_INFO_H
#define INCLUDED_GROUP_GROUP_INFO_H

#include "config.h"
#include "group/group_common.h"
#include "group/group_manager.fwd.h"
#include "group/region/group_region.h"
#include "group/group_msg.h"
#include "tree/tree.h"

namespace vt { namespace group {

static constexpr NodeType const min_spanning_tree_size = 3;
static constexpr NodeType const min_region_size = 1;
static constexpr size_t const max_region_list_size = 4;

struct Info {
  using RegionType = region::Region;
  using RegionPtrType = std::unique_ptr<RegionType>;
  using TreeType = Tree;
  using TreePtrType = std::unique_ptr<TreeType>;
  using ListType = std::vector<RegionType::BoundType>;

  Info(
    bool const& in_is_collective, RegionPtrType in_region, ActionType in_action,
    GroupType const in_group, RegionType::SizeType const& total_size,
    bool const& in_is_remote
  );

  friend struct GroupManager;

  template <typename MsgT>
  static void groupSetupHandler(MsgT* msg);
  static void groupTriggerHandler(GroupOnlyMsg* msg);

private:
  void setup();

private:
  bool is_collective_ = false;
  bool is_setup_ = false;
  bool is_forward_ = false;
  bool this_node_included_ = false;
  NodeType forward_node_ = uninitialized_destination;
  RegionPtrType region_;
  ActionType finished_setup_action_ = nullptr;
  TreePtrType default_spanning_tree_ = nullptr;
  GroupType const group_;
  RegionType::SizeType total_size_ = 0;
  RegionType::SizeType wait_count_ = 0;
  bool is_remote_ = false;
  ListType region_list_;
};

}} /* end namespace vt::group */

#include "group/group_info.impl.h"

#endif /*INCLUDED_GROUP_GROUP_INFO_H*/
