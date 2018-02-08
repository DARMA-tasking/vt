
#if !defined INCLUDED_GROUP_GROUP_MANAGER_H
#define INCLUDED_GROUP_GROUP_MANAGER_H

#include "config.h"
#include "group/group_common.h"
#include "group/region/group_region.h"
#include "group/group_info.fwd.h"
#include "group/group_manager.fwd.h"
#include "group/group_manager_active_attorney.fwd.h"
#include "group/group_msg.h"
#include "group/global/group_default.h"
#include "group/global/group_default_msg.h"
#include "registry/auto_registry_interface.h"
#include "messaging/message.h"
#include "messaging/active.h"
#include "activefn/activefn.h"
#include "collective/tree/tree.h"

#include <memory>
#include <unordered_map>
#include <cstdlib>

namespace vt { namespace group {

struct GroupManager {
  using RegionType = region::Region;
  using RegionPtrType = std::unique_ptr<RegionType>;
  using GroupInfoType = Info;
  using GroupInfoPtrType = std::unique_ptr<GroupInfoType>;
  using GroupContainerType = std::unordered_map<GroupType, GroupInfoPtrType>;
  using ActionListType = std::vector<ActionType>;
  using ActionContainerType = std::unordered_map<
    RemoteOperationIDType, ActionListType
  >;
  using ActionGroupType = std::function<void(GroupType)>;
  using TreeType = collective::tree::Tree;
  using TreePtrType = std::unique_ptr<TreeType>;

  GroupManager();

  void setupDefaultGroup();

  GroupType newGroup(
    RegionPtrType in_region, bool const& is_collective,
    bool const& is_static, ActionGroupType action
  );

  GroupType newGroup(RegionPtrType in_region, ActionGroupType action);

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  void sendMsg(GroupType const& group, MsgT* msg);

  friend struct Info;
  friend struct GroupActiveAttorney;

private:
  GroupType newCollectiveGroup(
    RegionPtrType in_region, bool const& is_static, ActionGroupType action
  );
  GroupType newLocalGroup(
    RegionPtrType in_region, bool const& is_static, ActionGroupType action
  );
  void initializeLocalGroup(
    GroupType const& group, RegionPtrType in_region, bool const& is_static,
    ActionType action, RegionType::SizeType const& group_size
  );
  void initializeRemoteGroup(
    GroupType const& group, RegionPtrType in_region, bool const& is_static,
    RegionType::SizeType const& group_size
  );
  RemoteOperationIDType registerContinuation(ActionType action);
  void registerContinuation(RemoteOperationIDType const& op, ActionType action);
  void triggerContinuation(RemoteOperationIDType const& op);

  EventType sendGroup(
    BaseMessage* base, NodeType const& from, MsgSizeType const& size,
    bool const is_root, ActionType action, bool* const deliver
  );

  static EventType groupHandler(
    BaseMessage* msg, NodeType const& from, MsgSizeType const& msg_size,
    bool const is_root, ActionType new_action, bool* const deliver
  );

private:
  GroupIDType next_group_id_ = initial_group_id;
  GroupContainerType local_group_info_;
  GroupContainerType remote_group_info_;
  RemoteOperationIDType cur_id_ = 0;
  ActionContainerType continuation_actions_;
};

}

extern group::GroupManager* theGroup();

} /* end namespace vt::group */

#include "group/group_manager.impl.h"

#endif /*INCLUDED_GROUP_GROUP_MANAGER_H*/
