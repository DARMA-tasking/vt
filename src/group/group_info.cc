
#include "config.h"
#include "group/group_common.h"
#include "group/group_info.h"
#include "group/id/group_id.h"
#include "group/region/group_region.h"
#include "group/group_msg.h"
#include "group/group_manager.h"
#include "configs/types/types_type.h"
#include "context/context.h"
#include "messaging/active.h"
#include "tree/tree.h"

#include <cassert>
#include <memory>

namespace vt { namespace group {

Info::Info(
  bool const& in_is_collective, RegionPtrType in_region, ActionType in_action,
  GroupType const in_group, RegionType::SizeType const& in_total_size,
  bool const& in_is_remote
) : is_collective_(in_is_collective), region_(std::move(in_region)),
    finished_setup_action_(in_action), group_(in_group),
    total_size_(in_total_size), is_remote_(in_is_remote)
{ }

void Info::setup() {
  auto const& size = region_->getSize();
  debug_print(
    group, node,
    "Info::setup: group size=%lu, is_remote=%s\n",
    size, print_bool(is_remote_)
  );
  if (is_remote_) {
    auto const& region_list = region_->makeList();
    this_node_included_ = true;
    default_spanning_tree_ = std::make_unique<TreeType>(region_list);
    is_setup_ = true;
  } else {
    assert(
      size >= min_region_size &&
      "Size of the region must be at least min_region_size"
    );
    if (is_collective_) {
      assert(0 && "Not implemented");
    } else {
      region_->sort();

      auto const& this_node = theContext()->getNode();
      auto const& region_list = region_->makeList();
      auto const& contains_this_node = region_->contains(this_node);
      if (size < min_spanning_tree_size && contains_this_node) {
        this_node_included_ = true;
        default_spanning_tree_ = std::make_unique<TreeType>(region_list);
        is_setup_ = true;
      } else {
        auto const& low_node = region_list[0];
        RemoteOperationIDType op = no_op_id;
        if (finished_setup_action_ != nullptr) {
          op = theGroup()->registerContinuation(finished_setup_action_);
        }
        auto msg = makeSharedMessage<GroupListMsg>(
          low_node, region_list.size(), group_, op, region_list.size(),
          this_node
        );
        msg->setPut(
          reinterpret_cast<void const*>(&region_list[0]),
          region_list.size()*sizeof(RegionType::BoundType)
        );
        is_forward_ = true;
        forward_node_ = low_node;
        theMsg()->sendMsg<GroupListMsg, Info::localGroupHandler>(
          low_node, msg
        );
      }
    }
  }
}

/*static*/ void Info::localGroupHandler(GroupListMsg* msg) {
  auto const& this_node = theContext()->getNode();
  auto const& group_size = msg->getCount();
  auto const& group_total_size = msg->getTotalCount();
  auto const& group_root = msg->getRoot();
  auto const& is_static = msg->isStatic();
  auto group = msg->getGroup();
  auto op_id = msg->getOpID();
  auto parent = msg->getParent();
  //auto const& group_create_node = GroupIDBuilder::getNode(group);
  auto const& ptr = static_cast<RegionType::BoundType*>(
    msg->getPut()
  );

  assert(
    this_node == group_root &&
    "This msg should only be sent to the root of the new local group"
  );

  debug_print(
    group, node,
    "Info::localGroupHandler: group size=%hd, group_total_size=%hd\n",
    group_size, group_total_size
  );

  for (int i = 0; i < group_size; i++) {
    debug_print(
      group, node,
      "Info::localGroupHandler: ptr[%d]=%d\n", i, ptr[i]
    );
  }

  if (group_size < min_spanning_tree_size) {
    auto region = std::make_unique<region::List>(ptr, group_total_size, true);

    theGroup()->initializeRemoteGroup(
      group, std::move(region), is_static, group_total_size
    );

    debug_print(
      group, node,
      "Info::localGroupHandler: op=%lu, parent=%d\n",
      op_id, parent
    );

    if (op_id != no_op_id) {
      // Send back message
      auto msg = makeSharedMessage<GroupOnlyMsg>(group, op_id);
      theMsg()->sendMsg<GroupOnlyMsg, Info::groupTriggerHandler>(parent, msg);
    }
  } else {
    // [0, 1,2,3,4,5,6,7,8]
    auto new_ptr = ptr + 1;
    auto new_size = group_size - 1;
    if (new_size < min_spanning_tree_size) {
      // Create parent region
      auto region = std::make_unique<region::List>(new_ptr, new_size, true);

      theGroup()->initializeRemoteGroup(
        group, std::move(region), is_static, group_total_size
      );
      // end parent region

      if (op_id != no_op_id) {
        auto msg = makeSharedMessage<GroupOnlyMsg>(group, op_id);
        theMsg()->sendMsg<GroupOnlyMsg, Info::groupTriggerHandler>(parent, msg);
      }

    } else {
      auto const& c1_size = new_size / 2;
      auto const& c2_size = new_size - c1_size;
      auto const& c1_ptr = new_ptr;
      auto const& c2_ptr = new_ptr + c1_size;
      auto const& c1 = *c1_ptr;
      auto const& c2 = *c2_ptr;
      auto region1 = std::make_unique<region::List>(c1_ptr, c1_size, true);
      auto region2 = std::make_unique<region::List>(c2_ptr, c2_size, true);

      debug_print(
        group, node,
        "Info::localGroupHandler: c1=%d, c2=%d, c1_size=%d, c2_size=%d\n",
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

      auto c1_msg = makeSharedMessage<GroupListMsg>(
        c1, c1_size, group, op1, group_total_size, this_node
      );
      auto c2_msg = makeSharedMessage<GroupListMsg>(
        c2, c2_size, group, op2, group_total_size, this_node
      );

      c1_msg->setPut(c1_ptr, c1_size * sizeof(RegionType::BoundType));
      c2_msg->setPut(c2_ptr, c2_size * sizeof(RegionType::BoundType));

      messageRef(msg);
      messageRef(msg);

      theMsg()->sendMsg<GroupListMsg, localGroupHandler>(c1, c1_msg);
      theMsg()->sendMsg<GroupListMsg, localGroupHandler>(c2, c2_msg);
    }
  }
}

/*static*/ void Info::groupTriggerHandler(GroupOnlyMsg* msg) {
  auto const& op_id = msg->getOpID();
  assert(op_id != no_op_id && "Must have valid op");
  theGroup()->triggerContinuation(op_id);
}

}} /* end namespace vt::group */
