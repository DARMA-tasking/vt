
#if !defined INCLUDED_GROUP_GROUP_INFO_H
#define INCLUDED_GROUP_GROUP_INFO_H

#include "config.h"
#include "group/group_common.h"
#include "group/group_info_collective.h"
#include "group/group_info_rooted.h"
#include "group/group_manager.fwd.h"
#include "group/region/group_region.h"
#include "group/group_msg.h"
#include "group/group_collective_msg.h"
#include "group/group_collective.h"
#include "collective/tree/tree.h"

#include <memory>
#include <cstdlib>

namespace vt { namespace group {

static constexpr NodeType const min_spanning_tree_size = 3;
static constexpr NodeType const min_region_size = 1;
static constexpr NodeType const default_num_children = 2;
static constexpr size_t const max_region_list_size = 4;

static struct InfoRootedConsType {} info_rooted_cons {};
static struct InfoRootedLocalConsType {} info_rooted_local_cons {};
static struct InfoRootedRemoteConsType {} info_rooted_remote_cons {};
static struct InfoCollectiveConsType {} info_collective_cons {};

struct Info : InfoRooted, InfoColl {
  using TreeType = collective::tree::Tree;
  using TreePtrType = std::unique_ptr<TreeType>;

protected:
  Info(
    bool const& in_is_collective, ActionType in_action,
    GroupType const in_group, bool const& in_is_remote,
    bool const& in_is_in_group, RegionPtrType in_region,
    RegionType::SizeType const& in_total_size
  );

  Info(
    InfoRootedConsType, RegionPtrType in_region, ActionType in_action,
    GroupType const in_group, RegionType::SizeType const& total_size,
    bool const& in_is_remote
  );

public:
  Info(
    InfoRootedLocalConsType, RegionPtrType in_region, ActionType in_action,
    GroupType const in_group, RegionType::SizeType const& total_size
  );

  Info(
    InfoRootedRemoteConsType, RegionPtrType in_region, GroupType const in_group,
    RegionType::SizeType const& total_size
  );

  Info(
    InfoCollectiveConsType, ActionType in_action, GroupType const in_group,
    bool const in_is_in_group
  );

  friend struct GroupManager;

  template <typename MsgT>
  static void groupSetupHandler(MsgT* msg);
  static void groupTriggerHandler(GroupOnlyMsg* msg);

protected:
  GroupType getGroupID() const override { return group_;                 }
  ActionType getAction() const override { return finished_setup_action_; }

private:
  void setup();

private:
  GroupType const group_;
  bool is_collective_               = false;
  ActionType finished_setup_action_ = nullptr;
};

}} /* end namespace vt::group */

#include "group/group_info.impl.h"

#endif /*INCLUDED_GROUP_GROUP_INFO_H*/
