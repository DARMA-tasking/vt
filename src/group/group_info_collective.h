
#if !defined INCLUDED_GROUP_GROUP_INFO_COLLECTIVE_H
#define INCLUDED_GROUP_GROUP_INFO_COLLECTIVE_H

#include "config.h"
#include "group/group_common.h"
#include "group/group_info_base.h"
#include "group/group_collective.h"
#include "group/group_collective_msg.h"
#include "group/group_collective_reduce_msg.h"
#include "collective/reduce/reduce.h"

#include <memory>
#include <list>

namespace vt { namespace group {

struct InfoColl : virtual InfoBase {
  using GroupCollectiveType = GroupCollective;
  using GroupCollectivePtrType = std::unique_ptr<GroupCollective>;
  using ReduceType = collective::reduce::Reduce;
  using ReducePtrType = ReduceType*;

  explicit InfoColl(bool const in_is_in_group)
    : is_in_group(in_is_in_group)
  { }

private:
  /*
   *  Inner struct used as functor reduce target after the collective group is
   *  fully created a operational: this will be triggered.
   */
  struct CollSetupFinished {
    void operator()(FinishedReduceMsg* msg);
  };

public:
  ReducePtrType getReduce() const;
  NodeType getRoot() const;
  bool isGroupDefault() const;
  TreeType* getTree() const;
  bool inGroup() const;
  bool isReady() const;
  void readyAction(ActionType const action);

protected:
  void setupCollective();
  void setupCollectiveSingular();

  static void upHan(GroupCollectiveMsg* msg);
  static void downHan(GroupCollectiveMsg* msg);
  static void newRootHan(GroupCollectiveMsg* msg);
  static void downFinishedHan(GroupOnlyMsg* msg);
  static void finalizeHan(GroupOnlyMsg* msg);
  static void newTreeHan(GroupOnlyMsg* msg);
  static void tree(GroupOnlyMsg* msg);

private:
  void upTree();
  void atRoot();
  void downTree(GroupCollectiveMsg* msg);
  void collectiveFn(GroupCollectiveMsg* msg);
  void newRoot(GroupCollectiveMsg* msg);
  void downTreeFinished(GroupOnlyMsg* msg);
  void finalizeTree(GroupOnlyMsg* msg);
  void finalize();
  void sendDownNewTree();
  void newTree(NodeType const& parent);
  RemoteOperationIDType makeCollectiveContinuation(GroupType const group_);

protected:
  bool is_in_group                       = false;
  bool finished_init_                    = false;
  bool in_phase_two_                     = false;
  GroupCollectivePtrType collective_     = nullptr;
  WaitCountType coll_wait_count_         = 0;
  std::vector<GroupCollectiveMsg*> msgs_ = {};
  uint32_t arrived_count_                = 0;
  uint32_t extra_count_                  = 0;
  uint32_t extra_arrived_count_          = 0;
  uint32_t send_down_                    = 0;
  uint32_t send_down_finished_           = 0;
  NodeType known_root_node_              = uninitialized_destination;
  bool is_new_root_                      = false;
  bool has_root_                         = false;
  bool is_default_group_                 = false;

private:
  RemoteOperationIDType down_tree_cont_     = no_op_id;
  RemoteOperationIDType down_tree_fin_cont_ = no_op_id;
  RemoteOperationIDType up_tree_cont_       = no_op_id;
  RemoteOperationIDType finalize_cont_      = no_op_id;
  RemoteOperationIDType new_tree_cont_      = no_op_id;
  RemoteOperationIDType new_root_cont_      = no_op_id;

private:
  std::list<ActionType> pending_ready_actions_ = {};
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_INFO_COLLECTIVE_H*/
