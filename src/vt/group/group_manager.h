/*
//@HEADER
// *****************************************************************************
//
//                               group_manager.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_GROUP_GROUP_MANAGER_H
#define INCLUDED_GROUP_GROUP_MANAGER_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/region/group_region.h"
#include "vt/group/group_info.fwd.h"
#include "vt/group/group_manager.fwd.h"
#include "vt/group/group_manager_active_attorney.fwd.h"
#include "vt/group/msg/group_msg.h"
#include "vt/group/global/group_default.h"
#include "vt/group/global/group_default_msg.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/messaging/message.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/messaging/active.h"
#include "vt/activefn/activefn.h"
#include "vt/collective/tree/tree.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/collective/collective_scope.h"
#include "vt/runtime/component/component_pack.h"

#include <memory>
#include <unordered_map>
#include <cstdlib>
#include <functional>

#include <mpi.h>

namespace vt { namespace group {

/**
 * \struct GroupManager
 *
 * \brief A core VT component that can create and manage groups of nodes.
 *
 * Enables rooted and collective asynchronous construction of groups of
 * nodes. The default group is all the nodes in the communicator, which is the
 * group a normal broadcast targets. Beyond this, both the VT system and users
 * can dynamically create groups in a fully distributed manner.
 */
struct GroupManager : runtime::component::Component<GroupManager> {
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
  using ReduceType = collective::reduce::Reduce;
  using ReducePtrType = ReduceType*;
  using CollectiveScopeType = collective::CollectiveScope;

  /**
   * \internal \brief Construct the GroupManager
   */
  GroupManager();

  virtual ~GroupManager() {
    for (auto&& elm : cleanup_actions_) {
      elm();
    }
    cleanup_actions_.clear();
  }

  std::string name() override { return "GroupManager"; }

  /**
   * \internal \brief Setup the default group spanning all nodes. Called by the
   * VT system at startup so broadcasts can take place.
   */
  void setupDefaultGroup();

  /**
   * \brief Create a new rooted group.
   *
   * \param[in] in_region list of nodes to include
   * \param[in] is_static whether the group is static after creation
   * \param[in] action action to execute when group is finished construction
   *
   * \return the group ID
   */
  GroupType newGroup(
    RegionPtrType in_region, bool const is_static, ActionGroupType action
  );

  /**
   * \brief Create a new rooted group
   *
   * \param[in] in_region list of nodes to include
   * \param[in] action action to execute when group is finished construction
   *
   * \return the group ID
   */
  GroupType newGroup(RegionPtrType in_region, ActionGroupType action);

  /**
   * \brief Create a group collectively, must be called on all nodes
   *
   * \param[in] in_group whether this node is included the new group
   * \param[in] action action to execute when group is finished construction
   * \param[in] make_mpi_group whether VT should create an underlying MPI group
   *
   * \return the group ID
   */
  GroupType newGroupCollective(
    bool const in_group, ActionGroupType action, bool make_mpi_group = false
  );

  /**
   * \internal \brief Generate the next group ID
   *
   * \param[in] GroupCollectiveLabelTagType tag to indicate collective
   *
   * \return the group ID
   */
  GroupType newGroupCollectiveLabel(GroupCollectiveLabelTagType);

  /**
   * \brief Query whether this node is part of a group
   *
   * \param[in] group the group ID
   *
   * \return whether this node is included
   */
  bool inGroup(GroupType const group);

  /**
   * \brief Get MPI_Comm from VT group
   *
   * \param[in] group_id the group ID
   *
   * \return the MPI_Comm associated with the group; returns \c MPI_COMM_WORLD
   * if group was created without a communicator
   */
  MPI_Comm getGroupComm(GroupType const group_id);

  template <typename MsgT, ActiveTypedFnType<MsgT> *f>
  void sendMsg(GroupType const group, MsgT* msg);

  friend struct Info;
  friend struct InfoColl;
  friend struct FinishedWork;
  friend struct InfoRooted;
  friend struct GroupActiveAttorney;

private:
  /**
   * \internal \brief System call to create a new collective group
   *
   * \param[in] in_group whether the node is in the group
   * \param[in] is_static whether the group is static after creation
   * \param[in] action action to execute when group is finished construction
   * \param[in] make_mpi_group whether VT should create an underlying MPI group
   *
   * \return the group ID
   */
  GroupType newCollectiveGroup(
    bool const in_group, bool const is_static, ActionGroupType action,
    bool make_mpi_group = false
  );

  /**
   * \internal \brief System call to create a new rooted group
   *
   * \param[in] in_region list of nodes in group
   * \param[in] is_static whether the group is static after creation
   * \param[in] action action to execute when group is finished construction
   *
   * \return the group ID
   */
  GroupType newLocalGroup(
    RegionPtrType in_region, bool const is_static, ActionGroupType action
  );

  /**
   * \internal \brief Setup new collective group
   *
   * \param[in] group the group ID
   * \param[in] is_static whether the group is static after creation
   * \param[in] action action to execute when group is finished construction
   * \param[in] in_group whether this node is included the new group
   * \param[in] make_mpi_group whether VT should create an underlying MPI group
   */
  void initializeLocalGroupCollective(
    GroupType const group, bool const is_static, ActionType action,
    bool const in_group, bool make_mpi_group
  );

  /**
   * \internal \brief Setup a new rooted group locally
   *
   * \param[in] group the group ID
   * \param[in] in_region list of nodes in group
   * \param[in] is_static whether the group is static after creation
   * \param[in] action action to execute when group is finished construction
   */
  void initializeLocalGroup(
    GroupType const group, RegionPtrType in_region, bool const is_static, ActionType action
  );

  /**
   * \internal \brief Setup a new rooted group remotely
   *
   * \param[in] group the group ID
   * \param[in] in_region list of nodes in group
   * \param[in] is_static whether the group is static after creation
   * \param[in] group_size the number of nodes in the group
   */
  void initializeRemoteGroup(
    GroupType const group, RegionPtrType in_region, bool const is_static,
    RegionType::SizeType const group_size
  );

  /**
   * \internal \brief Get new collective identifier for action registration
   *
   * \return the operation ID
   */
  RemoteOperationIDType nextCollectiveID() { return cur_collective_id_++; }

  /**
   * \internal \brief Collectively register a operation ID continuation
   *
   * \param[in] action the continuation
   *
   * \return the operation ID
   */
  RemoteOperationIDType registerContinuation(ActionType action);

  /**
   * \internal \brief Collectively register a operation ID continuation
   *
   * \param[in] op the operation ID
   * \param[in] action the continuation
   */
  void registerContinuation(
    RemoteOperationIDType const op, ActionType action
  );

  /**
   * \internal \brief Trigger a continuation with an operation ID
   *
   * \param[in] op the operation ID
   */
  void triggerContinuation(RemoteOperationIDType const op);

  /**
   * \internal \brief Send message to a rooted group
   *
   * \param[in] base the message to send
   * \param[in] from sender node
   * \param[in] size number of bytes of the message being sent
   * \param[in] is_root whether this node is the root
   * \param[out] deliver whether the caller should deliver locally
   *
   * \return the event ID for any generated events (like MPI_Requests)
   */
  EventType sendGroup(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const from,
    MsgSizeType const size, bool const is_root,
    bool* const deliver
  );

  /**
   * \internal \brief Send message to a collective group
   *
   * \param[in] base the message to send
   * \param[in] from sender node
   * \param[in] size number of bytes of the message being sent
   * \param[in] is_root whether this node is the root
   * \param[out] deliver whether the caller should deliver locally
   *
   * \return the event ID for any generated events (like MPI_Requests)
   */
  EventType sendGroupCollective(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const from,
    MsgSizeType const size, bool const is_root,
    bool* const deliver
  );

public:
  /**
   * \brief Get the reducer associated with a group to reduce over it
   *
   * \param[in] group the group ID
   *
   * \return pointer to the reducer
   */
  ReducePtrType groupReducer(GroupType const group);

  /**
   * \brief Get the root node for a group
   *
   * \param[in] group the group ID
   *
   * \return the root node
   */
  NodeType groupRoot(GroupType const group) const;

  /**
   * \brief Check if a group is the default group (all nodes, default spanning
   * tree)
   *
   * \param[in] group the group ID
   *
   * \return whether it is the default group
   */
  bool isGroupDefault(GroupType const group) const;

  /**
   * \internal \brief Add a cleanup action
   *
   * \param[in] action the action
   */
  void addCleanupAction(ActionType action);

  /**
   * \internal \brief Get the next operation ID
   *
   * \return the operation ID
   */
  RemoteOperationIDType getNextOpID();

private:
  /**
   * \internal \brief Execute necessary actions for deliver to a group when a
   * message arrives on this node
   *
   * \param[in] msg the message to send
   * \param[in] from sender node
   * \param[in] size number of bytes of the message being sent
   * \param[in] is_root whether this node is the root
   * \param[out] deliver whether the caller should deliver locally
   *
   * \return the event ID for any generated events (like MPI_Requests)
   */
  static EventType groupHandler(
    MsgSharedPtr<BaseMsgType> const& msg, NodeType const from,
    MsgSizeType const msg_size, bool const is_root,
    bool* const deliver
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
  ActionListType        cleanup_actions_              = {};
  CollectiveScopeType   collective_scope_;
};

/**
 * \internal \struct GroupManagerT
 *
 * \brief Group manager that handles typed actions that are registered for
 * system use.
 *
 * This is a separate template class because Intel 18 didn't like static members
 * that were variable templates. These are all the members of GroupManager that
 * were under a template <typename T>
 */
template <typename T>
struct GroupManagerT : public GroupManager
{
  using ActionTType = std::function<void(T)>;
  using ActionListTType = std::vector<ActionTType>;
  using ActionContainerTType = std::unordered_map<
    RemoteOperationIDType, ActionListTType
  >;

  GroupManagerT() = default;

  static void pushCleanupAction();
  static RemoteOperationIDType registerContinuationT(ActionTType action);
  static void registerContinuationT(RemoteOperationIDType const op, ActionTType a);
  static void triggerContinuationT(RemoteOperationIDType const op, T t);

private:
  static ActionContainerTType continuation_actions_t_;
  static std::unordered_map<RemoteOperationIDType,std::vector<T>> waiting_cont_;
};

} /* end namespace group */

extern group::GroupManager* theGroup();

} /* end namespace vt */

#include "vt/group/group_manager.impl.h"

#endif /*INCLUDED_GROUP_GROUP_MANAGER_H*/
