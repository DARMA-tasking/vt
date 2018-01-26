
#if !defined INCLUDED_GROUP_GROUP_INFO_IMPL_H
#define INCLUDED_GROUP_GROUP_INFO_IMPL_H

#include "config.h"
#include "group/group_common.h"
#include "group/group_info.h"
#include "group/id/group_id.h"
#include "group/region/group_region.h"
#include "group/region/group_range.h"
#include "group/region/group_list.h"
#include "group/region/group_shallow_list.h"
#include "group/group_msg.h"
#include "group/group_manager.h"
#include "configs/types/types_type.h"
#include "context/context.h"
#include "messaging/active.h"
#include "tree/tree.h"

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

  assert(
    this_node == group_root &&
    "This msg should only be sent to the root of the new local group"
  );

  auto new_size = group_size - 1;

  debug_print(
    group, node,
    "Info::groupSetupHandler: group size=%hd, group_total_size=%hd, "
    "new_size=%d, min_spanning_tree_size=%d\n",
    group_size, group_total_size, new_size, min_spanning_tree_size
  );

  if (new_size < min_spanning_tree_size) {
    debug_print(
      group, node,
      "Info::groupSetupHandler: create leaf node: size=%d\n",
      new_size
    );

    auto region = range.tail();
    auto owning_region = region->copy();

    theGroup()->initializeRemoteGroup(
      group, std::move(owning_region), is_static, group_total_size
    );

    debug_print(
      group, node,
      "Info::groupSetupHandler: op=%lu, parent=%d\n",
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
      "Info::groupSetupHandler: split and send to children: size=%d\n",
      new_size
    );

    auto const& range_tail = range.tail();
    auto const& split = range_tail->split();
    auto const& c1 = std::get<0>(split)->head();
    auto const& c2 = std::get<1>(split)->head();
    auto const& c1_size = std::get<0>(split)->getSize();
    auto const& c2_size = std::get<1>(split)->getSize();

    debug_print(
      group, node,
      "Info::groupSetupHandler: c1=%d, c2=%d, c1_size=%lu, c2_size=%lu\n",
      c1, c2, c1_size, c2_size
    );

    // Create parent region
    RegionType::BoundType list[2] = {c1,c2};
    auto region = std::make_unique<region::List>(list, 2, true);

    theGroup()->initializeRemoteGroup(
      group, std::move(region), is_static, group_size
    );
    // end parent region

    auto iter = theGroup()->remote_group_info_.find(group);
    assert(iter != theGroup()->remote_group_info_.end());
    auto info = iter->second.get();

    info->wait_count_ += 2;

    auto parent_cont = [msg,info,parent,group,op_id]{
      debug_print(
        group, node,
        "Info::parent continuation: wait_count_=%lu, parent=%d\n",
        info->wait_count_, parent
      );

      info->wait_count_--;
      if (info->wait_count_ == 0 && parent != uninitialized_destination) {
        debug_print(
          group, node,
          "Info::parent continuation: sending to parent=%d, op_id=%lu\n",
          parent, op_id
        );

        auto msg = makeSharedMessage<GroupOnlyMsg>(group, op_id);
        theMsg()->sendMsg<GroupOnlyMsg, Info::groupTriggerHandler>(
          parent, msg
        );
      }
      messageDeref(msg);
    };

    auto op1 = theGroup()->registerContinuation(parent_cont);
    auto op2 = theGroup()->registerContinuation(parent_cont);

    auto l1 = static_cast<RangeType*>(std::get<0>(split).get());
    auto l2 = static_cast<RangeType*>(std::get<1>(split).get());
    auto c1_msg = makeSharedMessage<MsgT>(
      c1, c1_size, group, op1, group_total_size, this_node, l1
    );
    auto c2_msg = makeSharedMessage<MsgT>(
      c2, c2_size, group, op2, group_total_size, this_node, l2
    );

    messageRef(msg);
    messageRef(msg);

    theMsg()->sendMsg<MsgT, groupSetupHandler>(c1, c1_msg);
    theMsg()->sendMsg<MsgT, groupSetupHandler>(c2, c2_msg);
  }
}

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_INFO_IMPL_H*/
