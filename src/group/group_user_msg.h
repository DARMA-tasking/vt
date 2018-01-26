
#if !defined INCLUDED_GROUP_GROUP_USER_MSG_H
#define INCLUDED_GROUP_GROUP_USER_MSG_H

#include "config.h"
#include "group/group_common.h"
#include "group/group_manager.fwd.h"

namespace vt { namespace group {

template <typename MsgT>
struct GroupUserMsg : MsgT {
  GroupUserMsg() = default;

  GroupType getGroup() const { return group_; }
  void setGroup(GroupType const& group) { group_ = group; }

  friend struct GroupManager;

private:
  HandlerType user_handler_ = uninitialized_handler;
  GroupType group_ = no_group;
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_USER_MSG_H*/
