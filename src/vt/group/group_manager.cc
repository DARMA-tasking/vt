/*
//@HEADER
// *****************************************************************************
//
//                               group_manager.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/group/group_common.h"
#include "vt/group/group_manager.h"
#include "vt/group/id/group_id.h"
#include "vt/group/region/group_region.h"
#include "vt/group/group_info.h"
#include "vt/group/global/group_default.h"
#include "vt/group/global/group_default_msg.h"
#include "vt/scheduler/scheduler.h"
#include "vt/collective/collective_alg.h"

#include <mpi.h>

namespace vt { namespace group {

GroupType GroupManager::newGroup(
  RegionPtrType in_region, bool const is_static, ActionGroupType action
) {
  return newLocalGroup(std::move(in_region), is_static, action);
}

GroupType GroupManager::newGroup(
  RegionPtrType in_region, ActionGroupType action
) {
  bool const is_static = true;
  return newGroup(std::move(in_region), is_static, action);
}

GroupType GroupManager::newGroupCollective(
  bool const in_group, ActionGroupType action, bool make_mpi_group
) {
  bool const is_static = true;
  return newCollectiveGroup(in_group, is_static, action, make_mpi_group);
}

GroupType GroupManager::newGroupCollectiveLabel(GroupCollectiveLabelTagType) {
  auto const& this_node = theContext()->getNode();
  auto next_id = next_collective_group_id_++;
  auto const& id = GroupIDBuilder::createGroupID(next_id,this_node,true,true);
  return id;
}

GroupType GroupManager::newCollectiveGroup(
  bool const is_in_group, bool const is_static, ActionGroupType action,
  bool make_mpi_group
) {
  auto const& this_node = theContext()->getNode();
  auto new_id = next_collective_group_id_++;
  bool const is_collective = true;
  auto const& group = GroupIDBuilder::createGroupID(
    new_id, this_node, is_collective, is_static
  );
  auto group_action = std::bind(action, group);
  initializeLocalGroupCollective(
    group, is_static, group_action, is_in_group, make_mpi_group
  );
  return group;
}

GroupType GroupManager::newLocalGroup(
  RegionPtrType in_region, bool const is_static, ActionGroupType action
) {
  auto const& this_node = theContext()->getNode();
  auto new_id = next_group_id_++;
  bool const is_collective = false;
  auto const& group = GroupIDBuilder::createGroupID(
    new_id, this_node, is_collective, is_static
  );
  auto group_action = std::bind(action, group);
  initializeLocalGroup(
    group, std::move(in_region), is_static, group_action
  );
  return group;
}

void GroupManager::deleteGroupCollective(GroupType group_id) {
  auto iter = local_collective_group_info_.find(group_id);
  if (iter != local_collective_group_info_.end()) {
    local_collective_group_info_.erase(iter);
  }
}

bool GroupManager::inGroup(GroupType const group) {
  auto iter = local_collective_group_info_.find(group);
  vtAssert(iter != local_collective_group_info_.end(), "Must exist");
  return iter->second->inGroup();
}

GroupManager::ReducePtrType GroupManager::groupReducer(GroupType const group) {
  auto iter = local_collective_group_info_.find(group);
  vtAssert(iter != local_collective_group_info_.end(), "Must exist");
  auto const& is_default_group = iter->second->isGroupDefault();
  if (!is_default_group) {
    return iter->second->getReduce();
  } else {
    return theCollective()->global();
  }
}

NodeType GroupManager::groupRoot(GroupType const group) const {
  auto iter = local_collective_group_info_.find(group);
  vtAssert(iter != local_collective_group_info_.end(), "Must exist");
  auto const& root = iter->second->getRoot();
  return root;
}

bool GroupManager::isGroupDefault(GroupType const group) const {
  auto iter = local_collective_group_info_.find(group);
  vtAssert(iter != local_collective_group_info_.end(), "Must exist");
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
  RemoteOperationIDType const op, ActionType action
) {
  continuation_actions_[op].push_back(action);
}

void GroupManager::triggerContinuation(RemoteOperationIDType const op) {
  auto iter = continuation_actions_.find(op);
  if (iter != continuation_actions_.end()) {
    for (auto&& elm : iter->second) {
      elm();
    }
    continuation_actions_.erase(iter);
  }
}

void GroupManager::initializeRemoteGroup(
  GroupType const group, RegionPtrType in_region, bool const is_static,
  RegionType::SizeType const group_size
) {
  auto group_info = std::make_unique<GroupInfoType>(
    info_rooted_remote_cons, default_comm_,
    std::move(in_region), group, group_size
  );
  auto group_ptr = group_info.get();
  remote_group_info_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(group),
    std::forward_as_tuple(std::move(group_info))
  );
  group_ptr->setup();
}

MPI_Comm GroupManager::getGroupComm(GroupType const group_id) {
  auto iter = local_collective_group_info_.find(group_id);
  vtAssert(
    iter != local_collective_group_info_.end(),
    "Must be a valid, active collection group to extract communicator"
  );
  // vtAssertInfo(
  //   iter != local_collective_group_info_.end(),
  //   "Must be a valid, active collection group to extract communicator",
  //   group_id
  // );
  if (iter != local_collective_group_info_.end()) {
    return iter->second->getComm();
  } else {
    return default_comm_;
  }
}

void GroupManager::initializeLocalGroupCollective(
  GroupType const group, bool const is_static, ActionType action,
  bool const in_group, bool make_mpi_group
) {
  auto group_info = std::make_unique<GroupInfoType>(
    info_collective_cons, default_comm_,
    action, group, in_group, make_mpi_group
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
  GroupType const group, RegionPtrType in_region, bool const is_static, ActionType action
) {

  auto const group_size = in_region->getSize();
  auto group_info = std::make_unique<GroupInfoType>(
    info_rooted_local_cons, default_comm_,
    std::move(in_region), action, group, group_size
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
  MsgSharedPtr<BaseMsgType> const& base, NodeType const from,
  bool const root, bool* const deliver
) {
  auto const& msg = reinterpret_cast<ShortMessage*>(base.get());
  auto const& is_pipe = envelopeIsPipe(msg->env);

  if (!is_pipe) {
    auto const& group = envelopeGetGroup(msg->env);
    auto const& is_bcast = envelopeIsBcast(msg->env);
    auto const& dest = envelopeGetDest(msg->env);

    vt_debug_print(
      normal, group,
      "GroupManager::groupHandler: size={}, root={}, from={}, group={:x}, "
      "bcast={}, dest={}\n",
      base.size(), root, from, group, is_bcast, dest
    );

    if (is_bcast) {
      // Deliver the message normally if it's not a the root of a broadcast
      *deliver = !root;
      if (group == default_group) {
        return global::DefaultGroup::broadcast(base,from,root,deliver);
      } else {
        auto const& is_collective_group = GroupIDBuilder::isCollective(group);
        if (is_collective_group) {
          return theGroup()->sendGroupCollective(base,from,root,deliver);
        } else {
          return theGroup()->sendGroup(base,from,root,deliver);
        }
      }
    } else {
      *deliver = true;
    }
    return no_event;
  } else {
    /*
     *  Pipe message bypass the group because they already specifically trigger
     *  actions that are not group-related but reuse the group ID as a the pipe
     *  ID to trigger corresponding callback actions
     */
    *deliver = true;
    return no_event;
  }
}

EventType GroupManager::sendGroupCollective(
  MsgSharedPtr<BaseMsgType> const& base, NodeType const from,
  bool const is_root,
  bool* const deliver
) {
  auto const& send_tag = static_cast<messaging::MPI_TagType>(
    messaging::MPITag::ActiveMsgTag
  );
  auto const& msg = base.get();
  auto const& group = envelopeGetGroup(msg->env);
  auto iter = local_collective_group_info_.find(group);
  vtAssert(iter != local_collective_group_info_.end(), "Must exist");
  auto const& info = *iter->second;
  auto const& in_group = info.inGroup();
  auto const& group_ready = info.isReady();

  if (info.isEmptyGroup()) {
    return no_event;
  }

  if (in_group && group_ready) {
    auto const& this_node = theContext()->getNode();
    auto const& dest = envelopeGetDest(msg->env);
    auto const& is_bcast = envelopeIsBcast(msg->env);
    auto const& is_group_collective = GroupIDBuilder::isCollective(group);
    auto const& root_node = info.getRoot();
    auto const& send_to_root = is_root && this_node != root_node;
    auto const& this_node_dest = dest == this_node;
    auto const& first_send = from == uninitialized_destination;

    if (root_node == uninitialized_destination) {
      return no_event;
    }

    vtAssert(is_group_collective, "This must be a collective group");

    vt_debug_print(
      terse, group,
      "GroupManager::sendGroupCollective: group={:x}, collective={}, "
      "in_group={}, group root={}, is_root={}, dest={}, from={}, is_bcast={}\n",
      group, is_group_collective, in_group, root_node, is_root, dest, from,
      is_bcast
    );

    EventType event = no_event;
    auto const& tree = info.getTree();
    auto const& num_children = tree->getNumChildren();

    vt_debug_print(
      normal, group,
      "GroupManager::sendGroupCollective: group={:x}, collective={}, "
      "num_children={}\n",
      group, is_group_collective, num_children
    );

    if ((num_children > 0 || send_to_root) && (!this_node_dest || first_send)) {
      info.getTree()->foreachChild([&](NodeType child){
        bool const& send = child != dest;

        vt_debug_print(
          normal, broadcast,
          "GroupManager::sendGroupCollective *send* size={}, from={}, child={}, "
          "send={}, msg={}\n",
          base.size(), from, child, send, print_ptr(msg)
        );

        if (send) {
          theMsg()->sendMsgBytesWithPut(child, base, send_tag);
        }
      });

      /*
       *  Send message to the root node of the group
       */
      if (send_to_root) {
        theMsg()->sendMsgBytesWithPut(root_node, base, send_tag);
      }

      if (!first_send && this_node_dest) {
        *deliver = false;
      } else {
        *deliver = true;
      }

      return event;
    } else {
      *deliver = true;
      return no_event;
    }
  } else if (in_group && !group_ready) {
    local_collective_group_info_.find(group)->second->readyAction(
      [base,from,is_root]{
        // Do not capture deliver, it's a pointer to stack memory
        bool dummy;
        theGroup()->sendGroupCollective(base,from,is_root,&dummy);
      }
    );
    *deliver = true;
    return no_event;
  } else {
    auto const& root_node = info.getRoot();

    if (root_node == uninitialized_destination) {
      return no_event;
    }

    vtAssert(!in_group, "Must not be in this group");
    /*
     *  Forward message to the root node of the group; currently, only nodes
     *  that are part of the group can be in the spanning tree. Thus, this node
     *  must forward.
     */
    auto const put_event = theMsg()->sendMsgBytesWithPut(
      root_node, base, send_tag
    );
    /*
     *  Do not deliver on this node since it is not part of the group and will
     *  just forward to the root node.
     */
    *deliver = false;
    return put_event;
  }
}

EventType GroupManager::sendGroup(
  MsgSharedPtr<BaseMsgType> const& base, NodeType const from,
  bool const is_root,
  bool* const deliver
) {
  auto const& this_node = theContext()->getNode();
  auto const& msg = base.get();
  auto const& group = envelopeGetGroup(msg->env);
  auto const& dest = envelopeGetDest(msg->env);
  auto const& this_node_dest = dest == this_node;
  auto const& first_send = from == uninitialized_destination;

  vt_debug_print(
    terse, group,
    "GroupManager::sendGroup: group={}, is_root={}\n",
    group, is_root
  );

  auto const& group_node = GroupIDBuilder::getNode(group);
  auto const& group_collective = GroupIDBuilder::isCollective(group);

  vtAssert(
    !group_collective, "Collective groups are not supported"
  );

  auto send_to_node = [&](NodeType node) -> EventType {
    auto const& send_tag = static_cast<messaging::MPI_TagType>(
      messaging::MPITag::ActiveMsgTag
    );

    return theMsg()->sendMsgBytesWithPut(node, base, send_tag);
  };

  EventType ret_event = no_event;

  if (is_root && group_node != this_node) {
    *deliver = false;
    return send_to_node(group_node);
  } else {
    auto iter = local_group_info_.find(group);
    bool is_at_root = iter != local_group_info_.end();
    if (is_at_root && iter->second->forward_node_ != this_node) {
      if (iter->second->forward_node_ != this_node) {
        auto& info = *iter->second;
        vtAssert(info.is_forward_, "Must be a forward");
        auto const& node = info.forward_node_;
        *deliver = false;
        return send_to_node(node);
      } else {
        *deliver = true;
        return no_event;
      }
    } else {
      auto remote_iter = remote_group_info_.find(group);

      vt_debug_print(
        normal, broadcast,
        "GroupManager::sendGroup: *send* remote size={}, from={}, found={}, "
        "dest={}, group={:x}, is_root={} \n",
        base.size(), from, remote_iter != remote_group_info_.end(), dest, group,
        is_root
      );

      if (remote_iter != remote_group_info_.end() && (!this_node_dest || first_send)) {
        auto& info = *remote_iter->second;
        vtAssert(!info.is_forward_, "Must not be a forward");
        vtAssert(
          info.default_spanning_tree_ != nullptr, "Must have spanning tree"
        );

        auto const& send_tag = static_cast<messaging::MPI_TagType>(
          messaging::MPITag::ActiveMsgTag
        );
        auto const& num_children = info.default_spanning_tree_->getNumChildren();

        // Send to child nodes in the group's spanning tree
        if (num_children > 0) {
          info.default_spanning_tree_->foreachChild([&](NodeType child) {
            vt_debug_print(
              verbose, broadcast,
              "GroupManager::sendGroup: *send* size={}, from={}, child={}\n",
              base.size(), from, child
            );

            if (child != this_node) {
              theMsg()->sendMsgBytesWithPut(child, base, send_tag);
            }
          });
        }

        if (is_root && from == uninitialized_destination) {
          *deliver = true;
        } else if (!is_root && dest == this_node) {
          *deliver = false;
        } else {
          *deliver = true;
        }
      }
    }
  }

  return ret_event;
}

GroupManager::GroupManager()
  : default_comm_(theContext()->getComm()),
    collective_scope_(theCollective()->makeCollectiveScope())
{
  global::DefaultGroup::setupDefaultTree();
}

void GroupManager::addCleanupAction(ActionType action) {
  cleanup_actions_.push_back(action);
}

RemoteOperationIDType GroupManager::getNextOpID(){
  return cur_id_++;
}

}} /* end namespace vt::group */
