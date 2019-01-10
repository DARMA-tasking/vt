/*
//@HEADER
// ************************************************************************
//
//                          group_info.impl.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_GROUP_GROUP_INFO_IMPL_H
#define INCLUDED_GROUP_GROUP_INFO_IMPL_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/group_info.h"
#include "vt/group/id/group_id.h"
#include "vt/group/region/group_region.h"
#include "vt/group/region/group_range.h"
#include "vt/group/region/group_list.h"
#include "vt/group/region/group_shallow_list.h"
#include "vt/group/msg/group_msg.h"
#include "vt/group/group_manager.h"
#include "vt/configs/types/types_type.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/collective/tree/tree.h"

#include <cassert>
#include <memory>

namespace vt { namespace group {

template <typename MsgT>
/*static*/ void Info::groupSetupHandler(MsgT* msg) {
  using RangeType = typename MsgT::RangeType;

  auto const& this_node = theContext()->getNode();
  auto const& group_size = msg->getCount();
  auto const& group_total_size = msg->getTotalCount();
  auto const& group_root = msg->getRoot();
  auto const& is_static = msg->isStatic();
  auto group = msg->getGroup();
  auto op_id = msg->getOpID();
  auto parent = msg->getParent();
  auto const& range = msg->getRange();

  vtAssert(
    this_node == group_root,
    "This msg should only be sent to the root of the new local group"
  );

  auto new_size = group_size - 1;

  debug_print(
    group, node,
    "Info::groupSetupHandler: group size={}, group_total_size={}, "
    "new_size={}, min_spanning_tree_size={}\n",
    group_size, group_total_size, new_size, min_spanning_tree_size
  );

  if (new_size < min_spanning_tree_size) {
    debug_print(
      group, node,
      "Info::groupSetupHandler: create leaf node: size={}\n",
      new_size
    );

    auto region = range.tail();
    auto owning_region = region->copy();

    theGroup()->initializeRemoteGroup(
      group, std::move(owning_region), is_static, group_total_size
    );

    debug_print(
      group, node,
      "Info::groupSetupHandler: op={}, parent={}\n",
      op_id, parent
    );

    if (op_id != no_op_id) {
      // Send back message
      auto msg = makeSharedMessage<GroupOnlyMsg>(group, op_id);
      theMsg()->sendMsg<GroupOnlyMsg, Info::groupTriggerHandler>(parent, msg);
    }
  } else {
    debug_print(
      group, node,
      "Info::groupSetupHandler: split and send to children: size={}\n",
      new_size
    );

    auto const& range_tail = range.tail();

    std::vector<RegionType::BoundType> local_nodes;

    auto parent_cont = [parent,group,op_id]{
      auto iter = theGroup()->remote_group_info_.find(group);
      vtAssertExpr(iter != theGroup()->remote_group_info_.end());
      auto info = iter->second.get();

      debug_print(
        group, node,
        "Info::parent continuation: wait_count_={}, parent={}\n",
        info->wait_count_, parent
      );

      info->wait_count_--;
      if (info->wait_count_ == 0 && parent != uninitialized_destination) {
        debug_print(
          group, node,
          "Info::parent continuation: sending to parent={}, op_id={}\n",
          parent, op_id
        );

        auto msg = makeSharedMessage<GroupOnlyMsg>(group, op_id);
        theMsg()->sendMsg<GroupOnlyMsg, Info::groupTriggerHandler>(
          parent, msg
        );
      }
    };

    range_tail->splitN(default_num_children, [&](RegionPtrType region){
      auto const& c = region->head();
      auto const& c_size = static_cast<NodeType>(region->getSize());

      local_nodes.push_back(c);

      debug_print(
        group, node,
        "Info::groupSetupHandler: child c={}, c_size={}\n", c, c_size
      );

      auto op1 = theGroup()->registerContinuation(parent_cont);
      auto l1 = static_cast<RangeType*>(region.get());
      auto c_msg = makeSharedMessage<MsgT>(
        c, c_size, group, op1, group_total_size, this_node, l1
      );
      theMsg()->sendMsg<MsgT, groupSetupHandler>(c, c_msg);
    });

    auto const& num_children = local_nodes.size();

    // Create parent region
    auto region = std::make_unique<region::List>(
      &local_nodes[0], num_children, true
    );

    theGroup()->initializeRemoteGroup(
      group, std::move(region), is_static, group_size
    );
    // end parent region

    auto iter = theGroup()->remote_group_info_.find(group);
    vtAssertExpr(iter != theGroup()->remote_group_info_.end());
    auto info = iter->second.get();
    info->wait_count_ += num_children;
  }
}

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_INFO_IMPL_H*/
