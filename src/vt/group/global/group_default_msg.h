
#if !defined INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_MSG_H
#define INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_MSG_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/messaging/message.h"

namespace vt { namespace group { namespace global {

struct GroupSyncMsg : ::vt::EpochTagMessage {
  GroupSyncMsg() = default;
};

}}} /* end namespace vt::group::global */

#endif /*INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_MSG_H*/
