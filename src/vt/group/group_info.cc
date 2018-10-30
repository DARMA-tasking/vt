
#include "config.h"
#include "configs/types/types_type.h"
#include "group/group_common.h"
#include "group/group_info.h"
#include "group/collective/group_info_collective.h"
#include "group/rooted/group_info_rooted.h"
#include "group/id/group_id.h"
#include "group/group_manager.h"
#include "context/context.h"
#include "messaging/active.h"
#include "collective/tree/tree.h"

#include <cassert>
#include <memory>

namespace vt { namespace group {

Info::Info(
  bool const& in_is_collective, ActionType in_action, GroupType const in_group,
  bool const& in_is_remote, bool const& in_is_in_group, RegionPtrType in_region,
  RegionType::SizeType const& in_total_size
) : InfoRooted(
      in_is_remote, in_region ? std::move(in_region) : nullptr, in_total_size
    ),
    InfoColl(
      in_is_in_group
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
      in_total_size
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
  bool const in_is_in_group
) : Info(true, in_action, in_group, false, in_is_in_group, nullptr, 0)
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
