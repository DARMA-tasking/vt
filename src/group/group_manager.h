
#if !defined INCLUDED_GROUP_GROUP_MANAGER_H
#define INCLUDED_GROUP_GROUP_MANAGER_H

#include "config.h"
#include "group/group_common.h"
#include "group/region/group_region.h"
#include "group/group_info.fwd.h"
#include "group/group_manager.fwd.h"
#include "registry/auto_registry_interface.h"
#include "messaging/message.h"
#include "messaging/active.h"
#include "activefn/activefn.h"

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

  GroupManager() = default;

  GroupType newGroup(
    RegionPtrType in_region, bool const& is_collective,
    bool const& is_static, ActionGroupType action
  );

  GroupType newGroup(RegionPtrType in_region, ActionGroupType action);

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  void sendMsg(GroupType const& group, MsgT* msg);

  friend struct Info;

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

  template <typename MsgT>
  void sendGroup(MsgT* msg, bool is_root);

  template <typename MsgT>
  static void groupForwardHandler(MsgT* msg);

  template <typename MsgT>
  static void groupSendHandler(MsgT* msg);

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
