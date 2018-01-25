
#if !defined INCLUDED_GROUP_GROUP_MANAGER_H
#define INCLUDED_GROUP_GROUP_MANAGER_H

#include "config.h"
#include "group/group_common.h"
#include "group/region/group_region.h"
#include "group/group_info.h"

#include <memory>
#include <unordered_map>

namespace vt { namespace group {

struct GroupManager {
  using RegionType = region::Region;
  using RegionPtrType = std::unique_ptr<RegionType>;
  using GroupInfoType = Info;
  using GroupInfoPtrType = std::unique_ptr<GroupInfoType>;
  using GroupContainerType = std::unordered_map<GroupType, GroupInfoPtrType>;

  GroupManager() = default;

  GroupType newGroup(
    RegionPtrType in_region, bool const& is_collective = false,
    bool const& is_static = true
  );

private:
  GroupType newCollectiveGroup(RegionPtrType in_region, bool const& is_static);
  GroupType newLocalGroup(RegionPtrType in_region, bool const& is_static);

private:
  GroupIDType next_group_id_ = initial_group_id;
  GroupContainerType local_group_info_;
};

}

extern group::GroupManager* theGroup();

} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_MANAGER_H*/
