
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
    region_list_ = region_->makeList();
    this_node_included_ = true;
    default_spanning_tree_ = std::make_unique<TreeType>(region_list_);
    is_setup_ = true;
  } else {
    assert(
      size >= min_region_size &&
      "Size of the region must be at least min_region_size"
    );
    if (is_collective_) {
      assert(0 && "Not implemented");
    } else {
      auto const& this_node = theContext()->getNode();

      region_->sort();
      if (region_->isList() || size < max_region_list_size) {
        auto const& contains_this_node = region_->contains(this_node);
        region_list_ = region_->makeList();
        if (size < min_spanning_tree_size && contains_this_node) {
          this_node_included_ = true;
          default_spanning_tree_ = std::make_unique<TreeType>(region_list_);
          is_setup_ = true;
          is_forward_ = true;
          forward_node_ = this_node;

          auto new_region = region_->copy();
          theGroup()->initializeRemoteGroup(
            group_, std::move(new_region), true, total_size_
          );

          if (finished_setup_action_) {
            finished_setup_action_();
          }
        } else {
          debug_print(
            group, node,
            "Info::setup: sending as list\n"
          );
          auto const& low_node = region_list_[0];
          RemoteOperationIDType op = no_op_id;
          if (finished_setup_action_ != nullptr) {
            op = theGroup()->registerContinuation(finished_setup_action_);
          }
          auto const& size = region_list_.size();
          region::ShallowList lst(region_list_);
          auto msg = makeSharedMessage<GroupListMsg>(
            low_node, size, group_, op, size, this_node, &lst
          );
          is_forward_ = true;
          forward_node_ = low_node;
          theMsg()->sendMsg<GroupListMsg, Info::groupSetupHandler>(
            low_node, msg
          );
        }
      } else {
        debug_print(
          group, node,
          "Info::setup: sending as range\n"
        );
        auto const& low_node = region_->head();
        RemoteOperationIDType op = no_op_id;
        if (finished_setup_action_ != nullptr) {
          op = theGroup()->registerContinuation(finished_setup_action_);
        }
        auto const& size = region_->getSize();
        auto msg = makeSharedMessage<GroupRangeMsg>(
          low_node, size, group_, op, size, this_node,
          static_cast<region::Range*>(region_.get())
        );
        is_forward_ = true;
        forward_node_ = low_node;
        theMsg()->sendMsg<GroupRangeMsg, Info::groupSetupHandler>(
          low_node, msg
        );
      }
    }
  }
}

/*static*/ void Info::groupTriggerHandler(GroupOnlyMsg* msg) {
  auto const& op_id = msg->getOpID();
  assert(op_id != no_op_id && "Must have valid op");
  theGroup()->triggerContinuation(op_id);
}

}} /* end namespace vt::group */
