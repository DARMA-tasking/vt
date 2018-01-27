
#if !defined INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H
#define INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H

#include "config.h"
#include "group/group_common.h"
#include "messaging/message.h"
#include "messaging/active.fwd.h"

namespace vt { namespace group {

struct GroupActiveAttorney {

  friend struct ::vt::ActiveMessenger;

private:
  static EventType groupHandler(
    BaseMessage* msg, NodeType const& from, MsgSizeType const& msg_size,
    bool const is_root, ActionType new_action, bool* const deliver
  );
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H*/
