
#include "config.h"
#include "group/group_common.h"
#include "group/group_info_base.h"
#include "group/group_info_collective.h"
#include "group/group_collective.h"
#include "group/group_collective_msg.h"
#include "context/context.h"
#include "messaging/active.h"
#include "collective/tree/tree.h"
#include "group/group_manager.h"

#include <memory>
#include <set>
#include <cstdlib>
#include <vector>
#include <cassert>

namespace vt { namespace group {

void InfoColl::setupCollective() {
  auto const& group_ = getGroupID();

  assert(!collective_   && "Collective should not be initialized");

  if (!collective_) {
    collective_ = std::make_unique<InfoColl::GroupCollectiveType>();
  }

  auto const& in_group = is_in_group;
  auto const& parent = collective_->getInitialParent();
  auto const& children = collective_->getInitialChildren();

  this->coll_wait_count_ = children + 1;

  debug_print(
    group, node,
    "InfoColl::setupCollective: is_in_group={}, group_={:x}, parent={}, "
    "children={}, wait={}\n",
    is_in_group, group_, parent, children, coll_wait_count_
  );

  up_tree_cont_   = makeCollectiveContinuation(group_);
  down_tree_cont_ = theGroup()->nextCollectiveID();
  theGroup()->registerContinuationT<GroupCollectiveMsg*>(
    down_tree_cont_,
    [group_](GroupCollectiveMsg* msg){
      auto iter = theGroup()->local_collective_group_info_.find(group_);
      assert(iter != theGroup()->local_collective_group_info_.end());
      iter->second->downTree(msg);
    }
  );

  debug_print(
    group, node,
    "InfoColl::setupCollective: is_in_group={}, parent={}: up tree\n",
    is_in_group, parent
  );

  auto const& size = is_in_group ? 1 : 0;
  auto const& child = theContext()->getNode();
  auto msg = makeSharedMessage<GroupCollectiveMsg>(
    group_, up_tree_cont_, in_group, size, child
  );
  theMsg()->sendMsg<GroupCollectiveMsg,upHan>(parent, msg);
}

void InfoColl::atRoot() {
  auto const& is_root = collective_->isInitialRoot();
  auto const& group = getGroupID();
  debug_print(
    group, node,
    "InfoColl::atRoot: is_in_group={}, group={:x}, root={}, children={}\n",
    is_in_group, group, is_root, collective_->span_children_.size()
  );
}

void InfoColl::upTree() {
  assert(msgs_.size() == coll_wait_count_ - 1 && "Must be equal");
  decltype(msgs_) msg_in_group = {};
  std::size_t subtree = 0;
  for (auto&& msg : msgs_) {
    if (msg->isInGroup()) {
      msg_in_group.push_back(msg);
      subtree += msg->getSubtreeSize();
    }
  }

  auto& span_children_ = collective_->span_children_;
  auto const& is_root = collective_->isInitialRoot();
  auto const& group = getGroupID();
  auto const& p = collective_->getInitialParent();
  auto const& op = up_tree_cont_;

  debug_print(
    group, node,
    "InfoColl::upTree: is_in_group={}, msg_in_group.size()={}, group={:x}, "
    "op={:x}, is_root={}\n",
    is_in_group, msg_in_group.size(), group, op, is_root
  );

  if (is_root) {
    for (auto&& msg : msg_in_group) {
      span_children_.push_back(msg->getChild());
    }
    return;
  }

  if (is_in_group && (msg_in_group.size() > 1 || msg_in_group.size() == 0)) {
    /*
     *  Case where we have an approx. a balanced tree: send up the tree like
     *  usual
     */
    auto const& child = theContext()->getNode();
    auto const& size = subtree + 1;
    auto const& level =
      msg_in_group.size() == 2 ? msg_in_group[0]->getLevel() + 1 : 0;
    auto msg = makeSharedMessage<GroupCollectiveMsg>(
      group,op,true,size,child,level
    );
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg);

    for (auto&& msg : msg_in_group) {
      span_children_.push_back(msg->getChild());
    }
  } else if (
    !is_in_group && (msg_in_group.size() == 1 || msg_in_group.size() == 2)
  ) {
    /*
     * Promote the non-null children to the child of the current parent
     * bypassing this node that is null: thus, forward the non-null's child's
     * message up the initial spanning tree
     */
    auto const& child = theContext()->getNode();
    auto msg = makeSharedMessage<GroupCollectiveMsg>(
      group,op,true,0,child,0,msg_in_group.size()
    );
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg);
    /*
     *  Forward all the children messages up the tree (up to 2 of them)
     */
    for (std::size_t i = 0; i < msg_in_group.size(); i++) {
      theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg_in_group[i]);
    }
  } else if (is_in_group && msg_in_group.size() == 1) {
    /*
     * In this case, we have two valid nodes in the tree (the current node and a
     * child of this current node); we should not make the child on this node a
     * child in the spanning tree because that will create a stick-like graph,
     * loosing efficiency!
     */
    auto const& child = theContext()->getNode();
    auto msg = makeSharedMessage<GroupCollectiveMsg>(group,op,true,1,child,0,1);
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg);
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg_in_group[0]);

    assert(msg_in_group.size() == 1);
    span_children_.push_back(msg_in_group[0]->getChild());
  } else {
    assert(!is_in_group && msg_in_group.size() > 2);
    assert(msg_in_group.size() == 3);

    std::set<GroupCollectiveMsg*, GroupCollSort> msg_set;
    for (auto&& msg : msg_in_group) {
      msg_set.emplace(msg);
    }

    auto const& extra = 2;
    auto const& child = theContext()->getNode();
    auto msg = makeSharedMessage<GroupCollectiveMsg>(
      group,op,true,0,child,0,extra
    );
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg);

    auto iter = msg_set.rbegin();
    auto iter_end = msg_set.rend();
    NodeType c[2];
    c[0] = (*iter)->getChild();
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, *iter++);
    c[1] = (*iter)->getChild();
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, *iter++);

    int32_t i = 0;
    while (iter != iter_end) {
      (*iter)->setOpID(up_tree_cont_);
      theMsg()->sendMsg<GroupCollectiveMsg,downHan>(c[i % 2],*iter);
      ++iter;
    }
  }
}

RemoteOperationIDType InfoColl::makeCollectiveContinuation(
  GroupType const group_
) {
  auto const& id = theGroup()->nextCollectiveID();
  theGroup()->registerContinuationT<GroupCollectiveMsg*>(
    id, [group_](GroupCollectiveMsg* msg){
      auto iter = theGroup()->local_collective_group_info_.find(group_);
      assert(iter != theGroup()->local_collective_group_info_.end());
      iter->second->collectiveFn(msg);
    }
  );
  return id;
}

void InfoColl::collectiveFn(GroupCollectiveMsg* msg) {
  messageRef(msg);
  msgs_.push_back(msg);

  auto const& child = msg->getChild();
  auto const& children = collective_->getChildren();
  auto const& is_root = collective_->isInitialRoot();
  bool found = false;
  for (auto&& c : children) {
    if (child == c) {
      found = true;
      break;
    }
  }
  if (found) {
    arrived_count_++;
    extra_count_ += msg->getExtraNodes();
  }
  if (coll_wait_count_ == arrived_count_ + extra_count_ + 1) {
    upTree();
    if (is_root) {
      atRoot();
    }
  }
}

/*static*/ void InfoColl::upHan(GroupCollectiveMsg* msg) {
  debug_print(
    group, node,
    "InfoColl::upHan: group={:x}, op={:x}, child={}\n",
    msg->getGroup(), msg->getOpID(), msg->getChild()
  );

  messageRef(msg);
  auto const& op_id = msg->getOpID();
  assert(op_id != no_op_id && "Must have valid op");
  theGroup()->triggerContinuationT<GroupCollectiveMsg*>(op_id,msg);
  //messageDeref(msg);
}

void InfoColl::downTree(GroupCollectiveMsg* msg) {
  assert(collective_ && "Must be valid");
  collective_->span_children_.push_back(msg->getChild());
}

/*static*/ void InfoColl::downHan(GroupCollectiveMsg* msg) {
  return upHan(msg);
}

}} /* end namespace vt::group */
