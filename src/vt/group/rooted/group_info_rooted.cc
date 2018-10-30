
#include "config.h"
#include "group/group_common.h"
#include "group/base/group_info_base.h"
#include "group/rooted/group_info_rooted.h"
#include "group/region/group_region.h"
#include "group/region/group_range.h"
#include "group/region/group_list.h"
#include "group/region/group_shallow_list.h"
#include "group/msg/group_msg.h"
#include "context/context.h"
#include "messaging/active.h"
#include "collective/tree/tree.h"
#include "group/group_manager.h"

namespace vt { namespace group {

InfoRooted::InfoRooted(
  bool const& in_is_remote, RegionPtrType in_region,
  RegionType::SizeType const& in_total_size
) : region_(in_region ? std::move(in_region) : nullptr),
    total_size_(in_total_size), is_remote_(in_is_remote)
{ }

void InfoRooted::setupRooted() {
  auto const& group_ = getGroupID();
  auto const& finished_setup_action_ = getAction();
  auto const& size = region_->getSize();
  debug_print(
    group, node,
    "Info::setupRooted: group size={}, is_remote={}\n",
    size, is_remote_
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
          "Info::setupRooted: sending as list\n"
        );
        auto const& low_node = region_list_[0];
        RemoteOperationIDType op = no_op_id;
        if (finished_setup_action_ != nullptr) {
          op = theGroup()->registerContinuation(finished_setup_action_);
        }
        auto const& size = static_cast<NodeType>(region_list_.size());
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
      auto const& size = static_cast<NodeType>(region_->getSize());
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


}} /* end namespace vt::group */
