
#if !defined INCLUDED_GROUP_GROUP_INFO_COLLECTIVE_H
#define INCLUDED_GROUP_GROUP_INFO_COLLECTIVE_H

#include "config.h"
#include "group/group_common.h"
#include "group/group_info_base.h"
#include "group/group_collective.h"
#include "group/group_collective_msg.h"

#include <memory>

namespace vt { namespace group {

struct InfoColl : virtual InfoBase {
  using GroupCollectiveType = GroupCollective;
  using GroupCollectivePtrType = std::unique_ptr<GroupCollective>;

  explicit InfoColl(bool const in_is_in_group)
    : is_in_group(in_is_in_group)
  { }

protected:
  void setupCollective();

  static void upHan(GroupCollectiveMsg* msg);
  static void downHan(GroupCollectiveMsg* msg);

private:
  void upTree();
  void atRoot();
  void downTree(GroupCollectiveMsg* msg);
  void collectiveFn(GroupCollectiveMsg* msg);
  RemoteOperationIDType makeCollectiveContinuation(GroupType const group_);

protected:
  bool is_in_group                       = false;
  bool finished_init_                    = false;
  GroupCollectivePtrType collective_     = nullptr;
  WaitCountType coll_wait_count_         = 0;
  std::vector<GroupCollectiveMsg*> msgs_ = {};
  uint32_t arrived_count_                = 0;
  uint32_t extra_count_                  = 0;

private:
  RemoteOperationIDType down_tree_cont_  = no_op_id;
  RemoteOperationIDType up_tree_cont_    = no_op_id;
};

struct GroupCollSort {
  bool operator()(
    GroupCollectiveMsg* const a, GroupCollectiveMsg* const b
  ) const {
    return a->getSubtreeSize() < b->getSubtreeSize();
  }
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_INFO_COLLECTIVE_H*/
