
#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/group_manager.h"
#include "vt/group/group_manager_active_attorney.h"

namespace vt { namespace group {

/*static*/ EventType GroupActiveAttorney::groupHandler(
  BaseMessage* msg, NodeType const& from, MsgSizeType const& msg_size,
  bool const is_root, ActionType new_action, bool* const deliver
) {
  return GroupManager::groupHandler(
    msg, from, msg_size, is_root, new_action, deliver
  );
}

}} /* end namespace vt::group */
