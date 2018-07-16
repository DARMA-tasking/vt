
#include "config.h"
#include "context/context.h"
#include "group/group_common.h"
#include "group/group_manager.h"
#include "group/id/group_id.h"
#include "group/region/group_region.h"
#include "group/group_info.h"
#include "group/global/group_default.h"
#include "group/global/group_default_msg.h"
#include "scheduler/scheduler.h"

namespace vt { namespace group {

GroupType GroupManager::newGroup(
  RegionPtrType in_region, bool const& is_collective, bool const& is_static,
  ActionGroupType action
) {
  assert(!is_collective && "Must not be collective");
  return newLocalGroup(std::move(in_region), is_static, action);
}

GroupType GroupManager::newGroup(
  RegionPtrType in_region, ActionGroupType action
) {
  bool const is_static = true;
  bool const is_collective = false;
  return newGroup(std::move(in_region), is_collective, is_static, action);
}

GroupType GroupManager::newGroupCollective(
  bool const in_group, ActionGroupType action
) {
  bool const is_static = true;
  return newCollectiveGroup(in_group, is_static, action);
}

GroupType GroupManager::newCollectiveGroup(
  bool const& is_in_group, bool const& is_static, ActionGroupType action
) {
  auto const& this_node = theContext()->getNode();
  auto new_id = next_collective_group_id_++;
  bool const is_collective = true;
  auto const& group = GroupIDBuilder::createGroupID(
    new_id, this_node, is_collective, is_static
  );
  auto group_action = std::bind(action, group);
  initializeLocalGroupCollective(group, is_static, group_action, is_in_group);
  return group;
}

GroupType GroupManager::newLocalGroup(
  RegionPtrType in_region, bool const& is_static, ActionGroupType action
) {
  auto const& this_node = theContext()->getNode();
  auto new_id = next_group_id_++;
  bool const is_collective = false;
  auto const& group = GroupIDBuilder::createGroupID(
    new_id, this_node, is_collective, is_static
  );
  auto const& size = in_region->getSize();
  auto group_action = std::bind(action, group);
  initializeLocalGroup(
    group, std::move(in_region), is_static, group_action, size
  );
  return group;
}

bool GroupManager::inGroup(GroupType const& group) {
  auto iter = local_collective_group_info_.find(group);
  assert(iter != local_collective_group_info_.end() && "Must exist");
  return iter->second->is_in_group;
}

GroupManager::ReducePtrType GroupManager::groupReduce(GroupType const& group) {
  auto iter = local_collective_group_info_.find(group);
  assert(iter != local_collective_group_info_.end() && "Must exist");
  return iter->second->getReduce();
}

NodeType GroupManager::groupRoot(GroupType const& group) const {
  auto iter = local_collective_group_info_.find(group);
  assert(iter != local_collective_group_info_.end() && "Must exist");
  auto const& root = iter->second->getRoot();
  assert(root != uninitialized_destination && "Must have valid root");
  return root;
}

bool GroupManager::groupDefault(GroupType const& group) const {
  auto iter = local_collective_group_info_.find(group);
  assert(iter != local_collective_group_info_.end() && "Must exist");
  auto const& def = iter->second->isGroupDefault();
  return def;
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
  bool const remote = true;
  auto group_info = std::make_unique<GroupInfoType>(
    info_rooted_remote_cons, std::move(in_region), group, group_size
  );
  auto group_ptr = group_info.get();
  remote_group_info_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(group),
    std::forward_as_tuple(std::move(group_info))
  );
  group_ptr->setup();
}

void GroupManager::initializeLocalGroupCollective(
  GroupType const& group, bool const& is_static, ActionType action,
  bool const in_group
) {
  auto group_info = std::make_unique<GroupInfoType>(
    info_collective_cons, action, group, in_group
  );
  auto group_ptr = group_info.get();
  local_collective_group_info_.emplace(
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
  bool const remote = false;
  auto group_info = std::make_unique<GroupInfoType>(
    info_rooted_local_cons, std::move(in_region), action, group, group_size
  );
  auto group_ptr = group_info.get();
  local_group_info_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(group),
    std::forward_as_tuple(std::move(group_info))
  );
  group_ptr->setup();
}

/*static*/ EventType GroupManager::groupHandler(
  BaseMessage* base, NodeType const& from, MsgSizeType const& size,
  bool const is_root, ActionType action, bool* const deliver
) {
  auto const& msg = reinterpret_cast<ShortMessage* const>(base);
  auto const& group = envelopeGetGroup(msg->env);
  auto const& is_bcast = envelopeIsBcast(msg->env);
  if (is_bcast) {
    // Deliver the message normally if it's not a the root of a broadcast
    *deliver = !is_root;
    if (group == default_group) {
      return global::DefaultGroup::broadcast(base, from, size, is_root, action);
    } else {
      return theGroup()->sendGroup(base, from, size, is_root, action, deliver);
    }
  } else {
    *deliver = true;
  }
  return no_event;
}

EventType GroupManager::sendGroup(
  BaseMessage* base, NodeType const& from, MsgSizeType const& size,
  bool const is_root, ActionType action, bool* const deliver
) {
  auto const& this_node = theContext()->getNode();
  auto const& msg = reinterpret_cast<ShortMessage* const>(base);
  auto const& group = envelopeGetGroup(msg->env);

  debug_print(
    group, node,
    "GroupManager::sendGroup: group={}, is_root={}\n",
    group, print_bool(is_root)
  );

  auto const& group_node = GroupIDBuilder::getNode(group);
  auto const& group_static = GroupIDBuilder::isStatic(group);
  auto const& group_collective = GroupIDBuilder::isCollective(group);
  auto const& send_tag = static_cast<messaging::MPI_TagType>(
    messaging::MPITag::ActiveMsgTag
  );

  assert(
    !group_collective && "Collective groups are not supported"
  );

  auto send_to_node = [&](NodeType node) -> EventType {
    EventType event = no_event;
    bool const& has_action = action != nullptr;
    EventRecordType* parent = nullptr;
    auto const& send_tag = static_cast<messaging::MPI_TagType>(
      messaging::MPITag::ActiveMsgTag
    );

    if (has_action) {
      event = theEvent()->createParentEvent(this_node);
      auto& holder = theEvent()->getEventHolder(event);
      parent = holder.get_event();
    }

    return theMsg()->sendMsgBytesWithPut(node, base, size, send_tag, action);
  };

  EventType ret_event = no_event;

  if (is_root && group_node != this_node) {
    *deliver = false;
    return send_to_node(group_node);
  } else {
    auto iter = local_group_info_.find(group);
    bool is_at_root = iter != local_group_info_.end();
    if (is_at_root && iter->second->forward_node_ != this_node) {
      auto& info = *iter->second;
      assert(info.is_forward_ && "Must be a forward");
      auto const& node = info.forward_node_;
      *deliver = false;
      return send_to_node(node);
    } else {
      auto iter = remote_group_info_.find(group);

      debug_print(
        broadcast, node,
        "GroupManager::broadcast *send* remote size={}, from={}, found={}\n",
        size, from, print_bool(iter != remote_group_info_.end())
      );

      if (iter != remote_group_info_.end()) {
        auto& info = *iter->second;
        assert(!info.is_forward_ && "Must not be a forward");
        assert(
          info.default_spanning_tree_ != nullptr && "Must have spanning tree"
        );

        bool const& has_action = action != nullptr;
        EventRecordType* parent = nullptr;
        auto const& send_tag = static_cast<messaging::MPI_TagType>(
          messaging::MPITag::ActiveMsgTag
        );
        auto const& num_children = info.default_spanning_tree_->getNumChildren();

        if (has_action) {
          ret_event = theEvent()->createParentEvent(this_node);
          auto& holder = theEvent()->getEventHolder(ret_event);
          parent = holder.get_event();
        }

        // Send to child nodes in the group's spanning tree
        if (num_children > 0) {
          info.default_spanning_tree_->foreachChild([&](NodeType child) {
            debug_print(
              broadcast, node,
              "GroupManager::broadcast *send* size={}, from={}, child={}\n",
              size, from, child
            );

            if (child != this_node) {
              auto const put_event = theMsg()->sendMsgBytesWithPut(
                child, base, size, send_tag, action
              );
              if (has_action) {
                parent->addEventToList(put_event);
              }
            }
          });
        }
      }
    }
  }

  return ret_event;
}

GroupManager::GroupManager() {
  global::DefaultGroup::setupDefaultTree();
}

}} /* end namespace vt::group */
