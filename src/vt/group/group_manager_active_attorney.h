
#if !defined INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H
#define INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H

#include "config.h"
#include "group/group_common.h"
#include "messaging/message/message.h"
#include "messaging/message.h"
#include "messaging/active.fwd.h"
#include "messaging/message/smart_ptr.h"

namespace vt { namespace group {

struct GroupActiveAttorney {

  friend struct ::vt::messaging::ActiveMessenger;

private:
  static EventType groupHandler(
    MsgSharedPtr<BaseMsgType> const& msg, NodeType const& from,
    MsgSizeType const& msg_size, bool const is_root, ActionType new_action,
    bool* const deliver
  );
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H*/
