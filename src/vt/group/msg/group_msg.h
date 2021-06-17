/*
//@HEADER
// *****************************************************************************
//
//                                 group_msg.h
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

#if !defined INCLUDED_GROUP_GROUP_MSG_H
#define INCLUDED_GROUP_GROUP_MSG_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/region/group_region.h"
#include "vt/group/region/group_range.h"
#include "vt/group/region/group_list.h"
#include "vt/group/region/group_shallow_list.h"
#include "vt/messaging/message.h"

namespace vt { namespace group {

template <typename MsgT>
struct GroupMsg : MsgT {
  GroupMsg() = default;

  GroupMsg(GroupType const& in_group, RemoteOperationIDType const& in_op)
    : MsgT(), group_(in_group), op_id_(in_op)
  { }

  GroupMsg(
    GroupType const& in_group, RemoteOperationIDType const& in_op,
    NodeType const& in_root, bool const& in_default_group
  ) : MsgT(), group_(in_group), op_id_(in_op), root_(in_root),
      default_group_(in_default_group)
  { }

  GroupType getGroup() const { return group_; }
  RemoteOperationIDType getOpID() const { return op_id_; }
  NodeType getRoot() const { return root_; }
  bool isDefault() const { return default_group_; }

  void setGroup(GroupType const& group) { group_ = group; }
  void setOpID(RemoteOperationIDType const& op) { op_id_ = op; }
  void setRoot(NodeType const& root) { root_ = root; }

protected:
  GroupType group_             = no_group;
  RemoteOperationIDType op_id_ = no_op_id;
  NodeType root_               = uninitialized_destination;
  bool default_group_          = false;
};

using GroupOnlyMsg = GroupMsg<::vt::Message>;

template <typename MsgT>
struct GroupInfoMsg : MsgT {
  GroupInfoMsg() = default;
  GroupInfoMsg(
    NodeType const& in_root_node, NodeType const& in_num_nodes,
    GroupType const& in_group, RemoteOperationIDType in_op,
    NodeType const& in_total_num_nodes,
    NodeType const& in_parent_node = uninitialized_destination
  ) : MsgT(in_group, in_op), root_node_(in_root_node),
      num_nodes_(in_num_nodes), total_num_nodes_(in_total_num_nodes),
      parent_node_(in_parent_node)
  { }

  NodeType getRoot() const { return root_node_; }
  NodeType getCount() const { return num_nodes_; }
  NodeType getTotalCount() const { return total_num_nodes_; }
  NodeType getParent() const { return parent_node_; }
  bool isStatic() const { return is_static_; }

private:
  NodeType root_node_       = uninitialized_destination;
  NodeType num_nodes_       = uninitialized_destination;
  NodeType total_num_nodes_ = uninitialized_destination;
  bool is_static_           = true;
  NodeType parent_node_     = uninitialized_destination;
};

struct GroupListMsg : GroupInfoMsg<GroupMsg<::vt::PayloadMessage>> {
  using RangeType = region::ShallowList;

  GroupListMsg() = default;
  GroupListMsg(
    NodeType const& in_root_node, NodeType const& in_num_nodes,
    GroupType const& in_group, RemoteOperationIDType in_op,
    NodeType const& in_total_num_nodes, NodeType const& in_parent_node,
    RangeType* in_range
  ) : GroupInfoMsg(
        in_root_node, in_num_nodes, in_group, in_op, in_total_num_nodes,
        in_parent_node
      )
  {
    setPut(
      in_range->getBound(),
      in_range->getSize() * sizeof(RangeType::BoundType)
    );
  }

  RangeType getRange() {
    auto const& ptr = static_cast<RangeType::BoundType*>(getPut());
    return region::ShallowList(ptr, getCount());
  }
};


struct GroupRangeMsg : GroupInfoMsg<GroupMsg<::vt::Message>> {
  using RangeType     = region::Range;
  using RangeDataType = region::RangeData;

  GroupRangeMsg() = default;
  GroupRangeMsg(
    NodeType const& in_root_node, NodeType const& in_num_nodes,
    GroupType const& in_group, RemoteOperationIDType in_op,
    NodeType const& in_total_num_nodes, NodeType const& in_parent_node,
    RangeType* in_range
  ) : GroupInfoMsg(
        in_root_node, in_num_nodes, in_group, in_op, in_total_num_nodes,
        in_parent_node
      ),
      group_range_(RangeDataType(*in_range))
  { }

  RangeType getRange() const { return group_range_.getRange(); }

private:
  RangeDataType group_range_;
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_MSG_H*/
