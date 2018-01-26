
#include "config.h"
#include "group/group_common.h"
#include "group/group_manager.h"
#include "group/group_manager_active_attorney.h"

namespace vt { namespace group {

/*static*/ void GroupActiveAttorney::groupHandler(
  BaseMessage* msg, MsgSizeType const& msg_size
) {
  return GroupManager::groupHandler(msg, msg_size);
}

}} /* end namespace vt::group */
