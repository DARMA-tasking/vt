/*
//@HEADER
// ************************************************************************
//
//                          group_info.h
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

#if !defined INCLUDED_GROUP_GROUP_INFO_H
#define INCLUDED_GROUP_GROUP_INFO_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/collective/group_info_collective.h"
#include "vt/group/rooted/group_info_rooted.h"
#include "vt/group/group_manager.fwd.h"
#include "vt/group/region/group_region.h"
#include "vt/group/msg/group_msg.h"
#include "vt/group/collective/group_collective_msg.h"
#include "vt/group/collective/group_collective.h"
#include "vt/collective/tree/tree.h"

#include <memory>
#include <cstdlib>

namespace vt { namespace group {

static constexpr NodeType const min_spanning_tree_size = 3;
static constexpr NodeType const min_region_size = 1;
static constexpr NodeType const default_num_children = 2;
static constexpr size_t const max_region_list_size = 4;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct InfoRootedConsType {} info_rooted_cons {};
static struct InfoRootedLocalConsType {} info_rooted_local_cons {};
static struct InfoRootedRemoteConsType {} info_rooted_remote_cons {};
static struct InfoCollectiveConsType {} info_collective_cons {};
#pragma GCC diagnostic pop

struct Info : InfoRooted, InfoColl {
  using TreeType = collective::tree::Tree;
  using TreePtrType = std::unique_ptr<TreeType>;

protected:
  Info(
    bool const& in_is_collective, ActionType in_action,
    GroupType const in_group, bool const& in_is_remote,
    bool const& in_is_in_group, RegionPtrType in_region,
    RegionType::SizeType const& in_total_size, bool make_mpi_group
  );

  Info(
    InfoRootedConsType, RegionPtrType in_region, ActionType in_action,
    GroupType const in_group, RegionType::SizeType const& total_size,
    bool const& in_is_remote
  );

public:
  Info(
    InfoRootedLocalConsType, RegionPtrType in_region, ActionType in_action,
    GroupType const in_group, RegionType::SizeType const& total_size
  );

  Info(
    InfoRootedRemoteConsType, RegionPtrType in_region, GroupType const in_group,
    RegionType::SizeType const& total_size
  );

  Info(
    InfoCollectiveConsType, ActionType in_action, GroupType const in_group,
    bool const in_is_in_group, bool make_mpi_group
  );

  friend struct GroupManager;

  template <typename MsgT>
  static void groupSetupHandler(MsgT* msg);
  static void groupTriggerHandler(GroupOnlyMsg* msg);

protected:
  GroupType getGroupID() const override { return group_;                 }
  ActionType getAction() const override { return finished_setup_action_; }

private:
  void setup();

private:
  GroupType const group_;
  bool is_collective_               = false;
  ActionType finished_setup_action_ = nullptr;
};

}} /* end namespace vt::group */

#include "vt/group/group_info.impl.h"

#endif /*INCLUDED_GROUP_GROUP_INFO_H*/
