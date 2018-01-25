
#include "config.h"
#include "context/context.h"
#include "group/group_common.h"
#include "group/group_manager.h"
#include "group/id/group_id.h"
#include "group/region/group_region.h"

namespace vt { namespace group {

GroupType GroupManager::newGroup(
  RegionPtrType in_region, bool const& is_collective, bool const& is_static
) {
  if (is_collective) {
    return newCollectiveGroup(std::move(in_region), is_static);
  } else {
    return newLocalGroup(std::move(in_region), is_static);
  }
}

GroupType GroupManager::newCollectiveGroup(
  RegionPtrType in_region, bool const& is_static
) {

}

GroupType GroupManager::newLocalGroup(
  RegionPtrType in_region, bool const& is_static
) {
  auto const& this_node = theContext()->getNode();
  auto new_id = next_group_id_++;
  auto const& group = GroupIDBuilder::createGroupID(
    new_id, this_node, false, is_static
  );
  auto group_info = std::make_unique<GroupInfoType>();
  local_group_info_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(group),
    std::forward_as_tuple(std::move(group_info))
  );
  return group;
}

}} /* end namespace vt::group */
