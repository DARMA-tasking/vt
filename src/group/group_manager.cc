
#include "config.h"
#include "context/context.h"
#include "group/group_common.h"
#include "group/group_manager.h"
#include "group/id/group_id.h"
#include "group/region/group_region.h"
#include "group/group_info.h"

namespace vt { namespace group {

GroupType GroupManager::newGroup(
  RegionPtrType in_region, bool const& is_collective, bool const& is_static,
  ActionGroupType action
) {
  if (is_collective) {
    return newCollectiveGroup(std::move(in_region), is_static, action);
  } else {
    return newLocalGroup(std::move(in_region), is_static, action);
  }
}

GroupType GroupManager::newGroup(RegionPtrType in_region, ActionGroupType action) {
  return newGroup(std::move(in_region), false, true, action);
}

GroupType GroupManager::newCollectiveGroup(
  RegionPtrType in_region, bool const& is_static, ActionGroupType action
) {

}

GroupType GroupManager::newLocalGroup(
  RegionPtrType in_region, bool const& is_static, ActionGroupType action
) {
  auto const& this_node = theContext()->getNode();
  auto new_id = next_group_id_++;
  auto const& group = GroupIDBuilder::createGroupID(
    new_id, this_node, false, is_static
  );
  auto const& size = in_region->getSize();
  auto group_action = std::bind(action, group);
  initializeLocalGroup(
    group, std::move(in_region), is_static, group_action, size
  );
  return group;
}

RemoteOperationIDType GroupManager::registerContinuation(ActionType action) {
  RemoteOperationIDType next_id = cur_id_++;
  continuation_actions_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(next_id),
    std::forward_as_tuple(ActionListType{action})
  );
  return next_id;
}

void GroupManager::registerContinuation(
  RemoteOperationIDType const& op, ActionType action
) {
  continuation_actions_[op].push_back(action);
}

void GroupManager::triggerContinuation(RemoteOperationIDType const& op) {
  auto iter = continuation_actions_.find(op);
  if (iter != continuation_actions_.end()) {
    for (auto&& elm : iter->second) {
      elm();
    }
    continuation_actions_.erase(iter);
  }
}

void GroupManager::initializeRemoteGroup(
  GroupType const& group, RegionPtrType in_region, bool const& is_static,
  RegionType::SizeType const& group_size
) {
  auto group_info = std::make_unique<GroupInfoType>(
    false, std::move(in_region), nullptr, group, group_size, true
  );
  auto group_ptr = group_info.get();
  remote_group_info_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(group),
    std::forward_as_tuple(std::move(group_info))
  );
  group_ptr->setup();
}

void GroupManager::initializeLocalGroup(
  GroupType const& group, RegionPtrType in_region, bool const& is_static,
  ActionType action, RegionType::SizeType const& group_size
) {
  auto group_info = std::make_unique<GroupInfoType>(
    false, std::move(in_region), action, group, group_size, false
  );
  auto group_ptr = group_info.get();
  local_group_info_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(group),
    std::forward_as_tuple(std::move(group_info))
  );
  group_ptr->setup();
}

}} /* end namespace vt::group */
