
#if !defined INCLUDED_GROUP_GROUP_COLLECTIVE_UTIL_H
#define INCLUDED_GROUP_GROUP_COLLECTIVE_UTIL_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/collective/group_collective_msg.h"

namespace vt { namespace group {

struct GroupCollSort {
  bool operator()(GroupCollectiveMsg* a, GroupCollectiveMsg* b) const;
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_COLLECTIVE_UTIL_H*/
