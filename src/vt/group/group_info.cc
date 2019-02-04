/*
//@HEADER
// ************************************************************************
//
//                          group_info.cc
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

#include "vt/config.h"
#include "vt/configs/types/types_type.h"
#include "vt/group/group_common.h"
#include "vt/group/group_info.h"
#include "vt/group/collective/group_info_collective.h"
#include "vt/group/rooted/group_info_rooted.h"
#include "vt/group/id/group_id.h"
#include "vt/group/group_manager.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/collective/tree/tree.h"

#include <cassert>
#include <memory>

namespace vt { namespace group {

Info::Info(
  bool const& in_is_collective, ActionType in_action, GroupType const in_group,
  bool const& in_is_remote, bool const& in_is_in_group, RegionPtrType in_region,
  RegionType::SizeType const& in_total_size, bool make_mpi_group
) : InfoRooted(
      in_is_remote, in_region ? std::move(in_region) : nullptr, in_total_size
    ),
    InfoColl(
      in_is_in_group, make_mpi_group
    ),
    group_(in_group), is_collective_(in_is_collective),
    finished_setup_action_(in_action)
{ }

Info::Info(
  InfoRootedConsType, RegionPtrType in_region, ActionType in_action,
  GroupType const in_group, RegionType::SizeType const& in_total_size,
  bool const& in_is_remote
) : Info(
      false, in_action, in_group, in_is_remote, false, std::move(in_region),
      in_total_size, false
    )
{ }

Info::Info(
  InfoRootedLocalConsType, RegionPtrType in_region, ActionType in_action,
  GroupType const in_group, RegionType::SizeType const& in_total_size
) : Info(
      info_rooted_cons, std::move(in_region), in_action, in_group,
      in_total_size, false
    )
{ }

Info::Info(
  InfoRootedRemoteConsType, RegionPtrType in_region, GroupType const in_group,
  RegionType::SizeType const& in_total_size
) : Info(
      info_rooted_cons, std::move(in_region), nullptr, in_group,
      in_total_size, true
    )
{ }

Info::Info(
  InfoCollectiveConsType, ActionType in_action, GroupType const in_group,
  bool const in_is_in_group, bool mpi
) : Info(true, in_action, in_group, false, in_is_in_group, nullptr, 0, mpi)
{ }

void Info::setup() {
  if (is_collective_) {
    vtAssert(is_collective_ , "Must be collective for this setup");
    vtAssert(!collective_   , "Collective should not be initialized");
    vtAssert(!is_remote_    , "Must not be remote for this setup");
    vtAssert(!region_       , "Region must be nullptr");
    return setupCollective();
  } else {
    vtAssert(!is_collective_, "Must not be collective for this setup");
    return setupRooted();
  }
}

/*static*/ void Info::groupTriggerHandler(GroupOnlyMsg* msg) {
  auto const& op_id = msg->getOpID();
  vtAssert(op_id != no_op_id, "Must have valid op");
  theGroup()->triggerContinuation(op_id);
}

}} /* end namespace vt::group */
