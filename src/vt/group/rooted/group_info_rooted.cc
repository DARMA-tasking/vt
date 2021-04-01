/*
//@HEADER
// *****************************************************************************
//
//                             group_info_rooted.cc
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

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/base/group_info_base.h"
#include "vt/group/rooted/group_info_rooted.h"
#include "vt/group/region/group_region.h"
#include "vt/group/region/group_range.h"
#include "vt/group/region/group_list.h"
#include "vt/group/region/group_shallow_list.h"
#include "vt/group/msg/group_msg.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/collective/tree/tree.h"
#include "vt/group/group_manager.h"

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
  vt_debug_print(
    terse, group,
    "Info::setupRooted: group size={}, is_remote={}\n",
    size, is_remote_
  );
  if (is_remote_) {
    region_list_ = region_->makeList();
    this_node_included_ = true;
    default_spanning_tree_ = std::make_unique<TreeType>(region_list_);
    is_setup_ = true;
  } else {
    vtAssert(
      size >= min_region_size,
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
        vt_debug_print(
          normal, group,
          "Info::setupRooted: sending as list\n"
        );
        auto const& low_node = region_list_[0];
        RemoteOperationIDType op = no_op_id;
        if (finished_setup_action_ != nullptr) {
          op = theGroup()->registerContinuation(finished_setup_action_);
        }
        auto const& listsize = static_cast<NodeType>(region_list_.size());
        region::ShallowList lst(region_list_);
        auto msg = makeMessage<GroupListMsg>(
          low_node, listsize, group_, op, listsize, this_node, &lst
        );
        is_forward_ = true;
        forward_node_ = low_node;
        if (this_node != low_node) {
          theMsg()->sendMsg<GroupListMsg, Info::groupSetupHandler>(low_node, msg);
        } else {
          Info::groupSetupHandler(msg.get());
        }
      }
    } else {
      vt_debug_print(
        normal, group,
        "Info::setup: sending as range\n"
      );
      auto const& low_node = region_->head();
      RemoteOperationIDType op = no_op_id;
      if (finished_setup_action_ != nullptr) {
        op = theGroup()->registerContinuation(finished_setup_action_);
      }
      auto const& regsize = static_cast<NodeType>(region_->getSize());
      auto msg = makeMessage<GroupRangeMsg>(
        low_node, regsize, group_, op, regsize, this_node,
        static_cast<region::Range*>(region_.get())
      );
      is_forward_ = true;
      forward_node_ = low_node;
      if (this_node != low_node) {
        theMsg()->sendMsg<GroupRangeMsg, Info::groupSetupHandler>(low_node, msg);
      } else {
        Info::groupSetupHandler(msg.get());
      }
    }
  }
}


}} /* end namespace vt::group */
