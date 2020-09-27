/*
//@HEADER
// *****************************************************************************
//
//                                group_info.cc
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

const bool InCollective = true;
const bool IsRemote = true;
const bool IsInGroup = true;
const bool MakeMpiGroup = true;

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
  InfoRootedLocalConsType, RegionPtrType in_region, ActionType in_action,
  GroupType const in_group, RegionType::SizeType const& in_total_size
) : Info(
      not InCollective, in_action, in_group, not IsRemote, not IsInGroup,
      std::move(in_region), in_total_size, not MakeMpiGroup
    )
{ }

Info::Info(
  InfoRootedRemoteConsType, RegionPtrType in_region, GroupType const in_group,
  RegionType::SizeType const& in_total_size
) : Info(
      not InCollective, nullptr, in_group, IsRemote, not IsInGroup,
      std::move(in_region), in_total_size, not MakeMpiGroup
    )
{ }

Info::Info(
  InfoCollectiveConsType, ActionType in_action, GroupType const in_group,
  bool const in_is_in_group, bool make_mpi_group
) : Info(
      InCollective, in_action, in_group, not IsRemote, in_is_in_group,
      nullptr, 0, make_mpi_group
    )
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
