
#if !defined INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H
#define INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H

#include "config.h"
#include "group/group_common.h"
#include "messaging/message.h"

namespace vt { namespace group {

struct GroupActiveAttorney {
  static void groupHandler(BaseMessage* msg, MsgSizeType const& msg_size);
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H*/
