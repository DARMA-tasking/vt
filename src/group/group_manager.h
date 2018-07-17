
#if !defined INCLUDED_GROUP_GROUP_MANAGER_H
#define INCLUDED_GROUP_GROUP_MANAGER_H

#include "config.h"
#include "group/group_common.h"
#include "group/region/group_region.h"
#include "group/group_info.fwd.h"
#include "group/group_manager.fwd.h"
#include "group/group_manager_active_attorney.fwd.h"
#include "group/msg/group_msg.h"
#include "group/global/group_default.h"
#include "group/global/group_default_msg.h"
#include "registry/auto/auto_registry_interface.h"
#include "messaging/message.h"
#include "messaging/active.h"
#include "activefn/activefn.h"
#include "collective/tree/tree.h"
#include "collective/reduce/reduce.h"

#include <memory>
#include <unordered_map>
#include <cstdlib>
#include <functional>

namespace vt { namespace group {

struct GroupManager {
  using RegionType = region::Region;
  using RegionPtrType = std::unique_ptr<RegionType>;
  using GroupInfoType = Info;
  using GroupInfoPtrType = std::unique_ptr<GroupInfoType>;
  using GroupContainerType = std::unordered_map<GroupType, GroupInfoPtrType>;
  using ActionGroupType = std::function<void(GroupType)>;
  using TreeType = collective::tree::Tree;
  using TreePtrType = std::unique_ptr<TreeType>;
  using ActionListType = std::vector<ActionType>;
  using ActionContainerType = std::unordered_map<
    RemoteOperationIDType, ActionListType
  >;
  template <typename T>
  using ActionTType = std::function<void(T)>;
  template <typename T>
  using ActionListTType = std::vector<ActionTType<T>>;
  template <typename T>
  using ActionContainerTType = std::unordered_map<
    RemoteOperationIDType, ActionListTType<T>
  >;
  using ReduceType = collective::reduce::Reduce;
  using ReducePtrType = ReduceType*;

  GroupManager();

  void setupDefaultGroup();

  GroupType newGroup(
    RegionPtrType in_region, bool const& is_collective,
    bool const& is_static, ActionGroupType action
  );

  GroupType newGroup(RegionPtrType in_region, ActionGroupType action);
  GroupType newGroupCollective(bool const in_group, ActionGroupType action);
  bool inGroup(GroupType const& group);

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  void sendMsg(GroupType const& group, MsgT* msg);

  friend struct Info;
  friend struct InfoColl;
  friend struct FinishedWork;
  friend struct InfoRooted;
  friend struct GroupActiveAttorney;

private:
  GroupType newCollectiveGroup(
    bool const& in_group, bool const& is_static, ActionGroupType action
  );
  GroupType newLocalGroup(
    RegionPtrType in_region, bool const& is_static, ActionGroupType action
  );
  void initializeLocalGroupCollective(
    GroupType const& group, bool const& is_static, ActionType action,
    bool const in_group
  );
  void initializeLocalGroup(
    GroupType const& group, RegionPtrType in_region, bool const& is_static,
    ActionType action, RegionType::SizeType const& group_size
  );
  void initializeRemoteGroup(
    GroupType const& group, RegionPtrType in_region, bool const& is_static,
    RegionType::SizeType const& group_size
  );

  template <typename T>
  RemoteOperationIDType registerContinuationT(ActionTType<T> action);
  template <typename T>
  void registerContinuationT(
    RemoteOperationIDType const& op, ActionTType<T> a
  );
  template <typename T>
  void triggerContinuationT(RemoteOperationIDType const& op, T t);
  RemoteOperationIDType nextCollectiveID() { return cur_collective_id_++; }

  RemoteOperationIDType registerContinuation(ActionType action);
  void registerContinuation(
    RemoteOperationIDType const& op, ActionType action
  );
  void triggerContinuation(RemoteOperationIDType const& op);

  EventType sendGroup(
    BaseMessage* base, NodeType const& from, MsgSizeType const& size,
    bool const is_root, ActionType action, bool* const deliver
  );

  EventType sendGroupCollective(
    BaseMessage* base, NodeType const& from, MsgSizeType const& size,
    bool const is_root, ActionType action, bool* const deliver
  );

public:
  ReducePtrType groupReduce(GroupType const& group);
  NodeType groupRoot(GroupType const& group) const;
  bool groupDefault(GroupType const& group) const;

private:
  static EventType groupHandler(
    BaseMessage* msg, NodeType const& from, MsgSizeType const& msg_size,
    bool const is_root, ActionType new_action, bool* const deliver
  );

private:
  GroupIDType           next_group_id_                = initial_group_id;
  GroupIDType           next_collective_group_id_     = initial_group_id;
  GroupContainerType    local_collective_group_info_  = {};
  GroupContainerType    local_group_info_             = {};
  GroupContainerType    remote_group_info_            = {};
  RemoteOperationIDType cur_id_                       = 0;
  RemoteOperationIDType cur_collective_id_            = 0xFFFFFFFF00000000;
  ActionContainerType   continuation_actions_         = {};

  template <typename T>
  static ActionContainerTType<T> continuation_actions_t_;
  template <typename T>
  static std::unordered_map<RemoteOperationIDType,std::vector<T>> waiting_cont_;
};

} /* end namespace group */

extern group::GroupManager* theGroup();

} /* end namespace vt */

#include "group/group_manager.impl.h"

#endif /*INCLUDED_GROUP_GROUP_MANAGER_H*/
