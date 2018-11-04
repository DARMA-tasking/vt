
#if !defined INCLUDED_GROUP_GROUP_COLLECTIVE_REDUCE_MSG_H
#define INCLUDED_GROUP_GROUP_COLLECTIVE_REDUCE_MSG_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/collective/group_collective_msg.h"
#include "vt/collective/reduce/reduce.h"

namespace vt { namespace group {

struct FinishedReduceMsg : collective::ReduceTMsg<collective::NoneType> {
  FinishedReduceMsg() = default;
  explicit FinishedReduceMsg(GroupType const& in_group)
    : group_(in_group)
  { }

  GroupType getGroup() const { return group_; }

private:
  GroupType group_ = no_group;
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_COLLECTIVE_REDUCE_MSG_H*/
