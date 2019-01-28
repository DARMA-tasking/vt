/*
//@HEADER
// ************************************************************************
//
//                          group_info_collective.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/base/group_info_base.h"
#include "vt/group/collective/group_info_collective.h"
#include "vt/group/collective/group_collective.h"
#include "vt/group/collective/group_collective_msg.h"
#include "vt/group/collective/group_collective_reduce_msg.h"
#include "vt/group/collective/group_collective_util.h"
#include "vt/group/group_manager.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/collective/tree/tree.h"
#include "vt/collective/collective_alg.h"
#include "vt/collective/collective_ops.h"

#include <memory>
#include <set>
#include <cstdlib>
#include <vector>
#include <list>
#include <algorithm>
#include <cassert>

#include <mpi.h>

namespace vt { namespace group {

void InfoColl::setupCollectiveSingular() {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& in_group = is_in_group;
  vtAssert(num_nodes == 1, "This method handles single node case");
  if (in_group) {
    known_root_node_   = this_node;
    is_new_root_       = true;
    in_phase_two_      = true;
    has_root_          = true;
    is_default_group_  = true;
  } else {
    known_root_node_   = uninitialized_destination;
    is_new_root_       = false;
    in_phase_two_      = true;
    has_root_          = true;
    is_default_group_  = false;
  }
  collective_->span_ = std::make_unique<TreeType>(
    true,uninitialized_destination,TreeType::NodeListType{}
  );
  auto const& action = getAction();
  if (action) {
    action();
  }
}

MPI_Comm InfoColl::getComm() const {
  return mpi_group_comm;
}

void InfoColl::freeComm() {
  if (mpi_group_comm != MPI_COMM_WORLD) {
    MPI_Comm_free(&mpi_group_comm);
    mpi_group_comm = MPI_COMM_WORLD;
  }
}

void InfoColl::setupCollective() {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& group_ = getGroupID();

  vtAssert(!collective_  , "Collective should not be initialized");

  if (!collective_) {
    collective_ = std::make_unique<InfoColl::GroupCollectiveType>();
  }

  if (num_nodes == 1) {
    return setupCollectiveSingular();
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

  if (make_mpi_comm) {
    auto const cur_comm = theContext()->getComm();
    int32_t const group_color = in_group;
    MPI_Comm_split(cur_comm, group_color, this_node, &mpi_group_comm);
  }

  up_tree_cont_       = makeCollectiveContinuation(group_);
  down_tree_cont_     = theGroup()->nextCollectiveID();
  down_tree_fin_cont_ = theGroup()->nextCollectiveID();
  finalize_cont_      = theGroup()->nextCollectiveID();
  new_tree_cont_      = theGroup()->nextCollectiveID();
  new_root_cont_      = theGroup()->nextCollectiveID();

  theGroup()->registerContinuationT<MsgSharedPtr<GroupCollectiveMsg>>(
    down_tree_cont_,
    [group_](MsgSharedPtr<GroupCollectiveMsg> msg){
      auto iter = theGroup()->local_collective_group_info_.find(group_);
      vtAssertExpr(iter != theGroup()->local_collective_group_info_.end());
      iter->second->downTree(msg.get());
    }
  );
  theGroup()->registerContinuationT<MsgSharedPtr<GroupOnlyMsg>>(
    down_tree_fin_cont_,
    [group_](MsgSharedPtr<GroupOnlyMsg> msg){
      auto iter = theGroup()->local_collective_group_info_.find(group_);
      vtAssertExpr(iter != theGroup()->local_collective_group_info_.end());
      iter->second->downTreeFinished(msg.get());
    }
  );
  theGroup()->registerContinuationT<MsgSharedPtr<GroupOnlyMsg>>(
    finalize_cont_,
    [group_](MsgSharedPtr<GroupOnlyMsg> msg){
      auto iter = theGroup()->local_collective_group_info_.find(group_);
      vtAssertExpr(iter != theGroup()->local_collective_group_info_.end());
      iter->second->finalizeTree(msg.get());
    }
  );
  theGroup()->registerContinuationT<MsgSharedPtr<GroupOnlyMsg>>(
    new_tree_cont_,
    [group_](MsgSharedPtr<GroupOnlyMsg> msg){
      auto iter = theGroup()->local_collective_group_info_.find(group_);
      vtAssertExpr(iter != theGroup()->local_collective_group_info_.end());
      auto const& from = theMsg()->getFromNodeCurrentHandler();
      iter->second->newTree(from);
    }
  );
  theGroup()->registerContinuationT<MsgSharedPtr<GroupCollectiveMsg>>(
    new_root_cont_,
    [group_](MsgSharedPtr<GroupCollectiveMsg> msg){
      auto iter = theGroup()->local_collective_group_info_.find(group_);
      vtAssertExpr(iter != theGroup()->local_collective_group_info_.end());
      auto const& from = theMsg()->getFromNodeCurrentHandler();
      iter->second->newRoot(msg.get());
    }
  );

  debug_print(
    group, node,
    "InfoColl::setupCollective: is_in_group={}, parent={}: up tree\n",
    is_in_group, parent
  );

  if (collective_->getInitialChildren() == 0) {
    auto const& size = static_cast<NodeType>(is_in_group ? 1 : 0);
    auto const& child = theContext()->getNode();
    auto msg = makeSharedMessage<GroupCollectiveMsg>(
      group_, up_tree_cont_, in_group, size, child
    );
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(parent, msg);
  }
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
  vtAssert(
    msgs_.size() - extra_count_  == coll_wait_count_ - 1, "Must be equal"
  );
  decltype(msgs_) msg_in_group = {};
  std::size_t subtree = 0;
  for (auto&& msg : msgs_) {
    if (msg->isInGroup()) {
      msg_in_group.push_back(msg);
      subtree += msg->getSubtreeSize();
    }
  }
  subtree_ = subtree;

  auto& span_children_ = collective_->span_children_;
  auto const& is_root = collective_->isInitialRoot();
  auto const& group = getGroupID();
  auto const& p = collective_->getInitialParent();
  auto const& op = up_tree_cont_;

  debug_print(
    group, node,
    "InfoColl::upTree: is_in_group={}, msgs.size()={}, msg_in_group.size()={}, "
    "extra={}, wait={}, group={:x}, op={:x}, is_root={}, subtree={}\n",
    is_in_group, msgs_.size(), msg_in_group.size(), extra_count_,
    coll_wait_count_, group, op, is_root, subtree
  );

  if (is_root) {
    if (is_in_group) {
      auto const& this_node = theContext()->getNode();
      for (auto&& msg : msg_in_group) {
        span_children_.push_back(msg->getChild());
      }
      /*
       *  In this case we have exactly the default group, so we can fall back to
       *  that without using this group explicitly
       */
      debug_print(
        group, node,
        "InfoColl::upTree: is_in_group={}, subtree={}, num_nodes={}\n",
        is_in_group, subtree, theContext()->getNumNodes()
      );
      if (subtree + 1 == theContext()->getNumNodes() && is_in_group) {
        /*
         *  This will allow bypassing using this spanning tree because it is
         *  equivalent in terms of functionality, although the spanning tree may
         *  differ
         */
        is_default_group_ = true;
      }
      known_root_node_ = this_node;
      is_new_root_     = true;
      in_phase_two_    = true;
      has_root_        = true;
      finalize();
      return;
    } else {
      if (msg_in_group.size() == 0) {
        auto const& group_ = getGroupID();
        vtAbort("A group must have at least a single node {}",group_);
      }
      /*
       *  Sort nodes to find the largest node to make it the root of the whole
       *  reduction
       */
      std::vector<GroupCollectiveMsg*> msg_list;
      for (auto&& msg : msg_in_group) {
        msg_list.emplace_back(msg);
      }
      std::sort(msg_list.begin(), msg_list.end(), GroupCollSort());

      auto const root_node = msg_list[0]->getChild();
      known_root_node_ = root_node;
      is_new_root_     = false;
      has_root_        = true;

      debug_print(
        group, node,
        "InfoColl::upTree: ROOT group={:x}, is_root={}, new_root={}, msgs={}\n",
        group, is_root, root_node, msg_list.size()
      );

      auto const& subtree = static_cast<NodeType>(0);
      auto const& extra = static_cast<GroupCollectiveMsg::CountType>(
        msg_in_group.size()-1
      );
      auto msg = makeSharedMessage<GroupCollectiveMsg>(
        group,new_root_cont_,true,subtree,root_node,0,extra
      );
      theMsg()->sendMsg<GroupCollectiveMsg,newRootHan>(root_node, msg);

      for (int i = 1; i < msg_list.size(); i++) {
        debug_print(
          group, node,
          "InfoColl::upTree: ROOT group={:x}, new_root={}, sending to={}\n",
          group, root_node, msg_list[i]->getChild()
        );

        msg_list[i]->setOpID(down_tree_cont_);
        theMsg()->sendMsg<GroupCollectiveMsg,downHan>(root_node,msg_list[i]);
        ++send_down_;
      }
      in_phase_two_ = true;
      finalize();
      return;
    }
  }

  auto const& subtree_this = is_in_group ? 1 : 0;
  auto const& sub = static_cast<NodeType>(subtree_this);

  if (is_in_group && (msg_in_group.size() == 2 || msg_in_group.size() == 0)) {
    /*
     *  Case where we have an approx. a balanced tree: send up the tree like
     *  usual
     */
    auto const& child = theContext()->getNode();
    auto const& total_subtree = static_cast<NodeType>(subtree + sub);
    auto const& level =
      msg_in_group.size() == 2 ? msg_in_group[0]->getLevel() + 1 : 0;
    auto msg = makeSharedMessage<GroupCollectiveMsg>(
      group,op,is_in_group,total_subtree,child,level
    );
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg);

    for (auto&& msg : msg_in_group) {
      span_children_.push_back(msg->getChild());
    }
  } else if (!is_in_group && msg_in_group.size() < 3) {
    /*
     * Promote the non-null children to the child of the current parent
     * bypassing this node that is null: thus, forward the non-null's child's
     * message up the initial spanning tree
     */
    auto const& child = theContext()->getNode();
    auto const& extra = static_cast<GroupCollectiveMsg::CountType>(
      msg_in_group.size()
    );
    auto msg = makeSharedMessage<GroupCollectiveMsg>(
      group,op,is_in_group,sub,child,0,extra
    );
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg);
    /*
     *  Forward all the children messages up the tree (up to 2 of them)
     */
    for (std::size_t i = 0; i < msg_in_group.size(); i++) {
      theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg_in_group[i].get());
    }
  } else if (is_in_group && msg_in_group.size() == 1) {
    /*
     * In this case, we have two valid nodes in the tree (the current node and a
     * child of this current node); we should not make the child on this node a
     * child in the spanning tree because that will create a stick-like graph,
     * loosing efficiency!
     */
    auto const& child = theContext()->getNode();
    auto const& extra = 1;
    auto msg = makeSharedMessage<GroupCollectiveMsg>(
      group,op,is_in_group,sub,child,0,extra
    );
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg);
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg_in_group[0].get());
  } else {
    vtAssertExpr(msg_in_group.size() > 2);

    std::vector<GroupCollectiveMsg*> msg_list;
    for (auto&& msg : msg_in_group) {
      debug_print(
        group, node,
        "InfoColl::upTree: inserting into set msg={}\n",
        print_ptr(msg.get())
      );
      msg_list.emplace_back(msg);
    }

    auto const& extra =
      static_cast<GroupCollectiveMsg::CountType>(msg_in_group.size() / 2);
    auto const& child = theContext()->getNode();
    auto const& total_subtree = static_cast<NodeType>(sub + subtree);
    auto msg = makeSharedMessage<GroupCollectiveMsg>(
      group,op,is_in_group,total_subtree,child,0,extra
    );
    theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, msg);

    debug_print(
      group, node,
      "InfoColl::upTree: msg_in_group.size()={}, msg_size.size()={}\n",
      msg_in_group.size(), msg_list.size()
    );

    std::sort(msg_list.begin(), msg_list.end(), GroupCollSort());

    auto iter = msg_list.rbegin();
    auto iter_end = msg_list.rend();
    NodeType* c = static_cast<NodeType*>(std::malloc(sizeof(NodeType) * extra));

    for (int i = 0; i < extra; i++) {
      c[i] = (*iter)->getChild();
      theMsg()->sendMsg<GroupCollectiveMsg,upHan>(p, *iter);
      iter++;
    }

    int32_t i = 0;
    while (iter != iter_end) {
      (*iter)->setOpID(down_tree_cont_);
      theMsg()->sendMsg<GroupCollectiveMsg,downHan>(c[i % extra],*iter);
      ++send_down_;
      ++iter;
    }

    std::free(c);
  }
}

RemoteOperationIDType InfoColl::makeCollectiveContinuation(
  GroupType const group_
) {
  auto const& id = theGroup()->nextCollectiveID();
  theGroup()->registerContinuationT<MsgSharedPtr<GroupCollectiveMsg>>(
    id, [group_](MsgSharedPtr<GroupCollectiveMsg> msg){
      auto iter = theGroup()->local_collective_group_info_.find(group_);
      vtAssertExpr(iter != theGroup()->local_collective_group_info_.end());
      iter->second->collectiveFn(msg);
    }
  );
  return id;
}

void InfoColl::newRoot(GroupCollectiveMsg* msg) {
  auto const& this_node = theContext()->getNode();

  debug_print(
    group, node,
    "InfoColl::newRoot: group={:x}\n", msg->getGroup()
  );

  known_root_node_ = this_node;
  has_root_        = true;
  is_new_root_     = true;
}

NodeType InfoColl::getRoot() const {
  debug_print(
    verbose, group, node,
    "InfoColl::getRoot: group={:x}, has_root_={}, known_root_node_={}\n",
    getGroupID(), has_root_, known_root_node_
  );

  if (!has_root_) {
    return uninitialized_destination;
  } else {
    return known_root_node_;
  }
}

bool InfoColl::isGroupDefault() const {
  return is_default_group_;
}

void InfoColl::collectiveFn(MsgSharedPtr<GroupCollectiveMsg> msg) {
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
  } else {
    extra_arrived_count_++;
  }

  auto const& ready =
    coll_wait_count_ + extra_count_ == arrived_count_ + extra_arrived_count_ + 1;

  debug_print(
    group, node,
    "InfoColl::collectiveFn: group={:x}, arrived_count_={}, extra_count_={}, "
    "coll_wait_count_-1={}, ready={}\n",
    msg->getGroup(), arrived_count_, extra_count_, coll_wait_count_-1,
    ready
  );

  if (ready) {
    upTree();
    if (is_root) {
      atRoot();
    }
  }
}

/*static*/ void InfoColl::tree(GroupOnlyMsg* msg) {
  auto const& op_id = msg->getOpID();
  vtAssert(op_id != no_op_id, "Must have valid op");
  auto msg_ptr = promoteMsg(msg);
  theGroup()->triggerContinuationT<MsgSharedPtr<GroupOnlyMsg>>(op_id,msg_ptr);
}

/*static*/ void InfoColl::upHan(GroupCollectiveMsg* msg) {
  debug_print(
    group, node,
    "InfoColl::upHan: group={:x}, op={:x}, child={}, extra={}\n",
    msg->getGroup(), msg->getOpID(), msg->getChild(), msg->getExtraNodes()
  );
  auto const& op_id = msg->getOpID();
  vtAssert(op_id != no_op_id, "Must have valid op");
  auto msg_ptr = promoteMsg(msg);
  theGroup()->triggerContinuationT<MsgSharedPtr<GroupCollectiveMsg>>(
    op_id,msg_ptr
  );
}

void InfoColl::downTree(GroupCollectiveMsg* msg) {
  auto const& from = theMsg()->getFromNodeCurrentHandler();

  debug_print(
    group, node,
    "InfoColl::downTree: group={:x}, child={}, from={}\n",
    getGroupID(), msg->getChild(), from
  );

  vtAssert(collective_, "Must be valid");

  if (collective_->span_children_.size() < 4) {
    collective_->span_children_.push_back(msg->getChild());
  } else {
    auto const& num = collective_->span_children_.size();
    auto const& child = collective_->span_children_[msg->getChild() % num];
    auto nmsg = makeSharedMessage<GroupCollectiveMsg>(*msg);
    theMsg()->sendMsg<GroupCollectiveMsg,downHan>(child,nmsg);
    ++send_down_;
  }

  auto const& group_ = getGroupID();
  auto nmsg = makeSharedMessage<GroupOnlyMsg>(group_,down_tree_fin_cont_);
  theMsg()->sendMsg<GroupOnlyMsg,downFinishedHan>(from,nmsg);
}

void InfoColl::newTree(NodeType const& parent) {
  auto const& group_ = getGroupID();
  collective_->parent_ = is_new_root_ ? -1 : parent;
  sendDownNewTree();
  vtAssert(is_in_group, "Must be in group");
  auto const& is_root = is_new_root_;
  collective_->span_   = std::make_unique<TreeType>(
    is_root, collective_->parent_, collective_->span_children_
  );
  collective_->reduce_ = std::make_unique<ReduceType>(
    group_, collective_->span_.get()
  );
  auto const& action = getAction();
  if (action) {
    action();
  }
  auto cur_actions = pending_ready_actions_;
  pending_ready_actions_.clear();
  for (auto&& act : cur_actions) {
    act();
  }
}

void InfoColl::sendDownNewTree() {
  auto const& group_ = getGroupID();
  auto const& children = collective_->span_children_;
  for (auto&& c : children) {
    debug_print(
      group, node,
      "InfoColl::sendDownNewTree: group={:x}, sending to child={}\n",
      group_, c
    );
    auto msg = makeSharedMessage<GroupOnlyMsg>(group_,new_tree_cont_);
    theMsg()->sendMsg<GroupOnlyMsg,newTreeHan>(c,msg);
  }
}

void InfoColl::finalize() {
  auto const& group_ = getGroupID();

  if (in_phase_two_ && send_down_finished_ == send_down_) {

      #if backend_debug_enabled(group)
        char buf[256];
        buf[0] = '\0';
        int cur = 0;
        for (auto&& elm : collective_->span_children_) {
          cur += sprintf(buf + cur, "%d,", elm);
        }

        auto const& num_children = collective_->span_children_.size();
        debug_print(
          group, node,
          "InfoColl::finalize: group={:x}, send_down_={}, "
          "send_down_finished_={}, in_phase_two_={}, in_group={}, "
          "has_root_={}, known_root_node_={}, is_new_root_={}, num={}, sub={},"
          "children={}\n",
          group_, send_down_, send_down_finished_, in_phase_two_, is_in_group,
          has_root_, known_root_node_, is_new_root_, num_children, subtree_, buf
        );
     #endif

    auto const& children = collective_->getChildren();
    for (auto&& c : children) {

      debug_print(
        group, node,
        "InfoColl::finalize: group={:x}, sending to child={}\n",
        group_, c
      );

      auto msg = makeSharedMessage<GroupOnlyMsg>(
        group_,finalize_cont_,known_root_node_,is_default_group_
      );
      theMsg()->sendMsg<GroupOnlyMsg,finalizeHan>(c,msg);
    }

    if (!is_in_group) {
      auto const& action = getAction();
      if (action) {
        action();
      }
    }

    auto const& root = 0;
    auto msg = makeSharedMessage<FinishedReduceMsg>(group_);
    theCollective()->reduce<
      FinishedReduceMsg,
      FinishedReduceMsg::msgHandler<
        FinishedReduceMsg,
        collective::PlusOp<collective::NoneType>,
        CollSetupFinished
      >
    >(root,msg);
  }
}

void InfoColl::finalizeTree(GroupOnlyMsg* msg) {
  auto const& new_root = msg->getRoot();
  vtAssert(new_root != uninitialized_destination, "Must have root node");
  debug_print(
    verbose, group, node,
    "InfoColl::finalizeTree: group={:x}, new_root={}\n",
    msg->getGroup(), new_root
  );
  in_phase_two_     = true;
  known_root_node_  = new_root;
  has_root_         = true;
  is_default_group_ = msg->isDefault();
  finalize();
}

void InfoColl::downTreeFinished(GroupOnlyMsg* msg) {
  send_down_finished_++;
  finalize();
}

/*static*/ void InfoColl::downHan(GroupCollectiveMsg* msg) {
  return upHan(msg);
}

/*static*/ void InfoColl::newRootHan(GroupCollectiveMsg* msg) {
  return upHan(msg);
}

/*static*/ void InfoColl::downFinishedHan(GroupOnlyMsg* msg) {
  return tree(msg);
}

/*static*/ void InfoColl::finalizeHan(GroupOnlyMsg* msg) {
  return tree(msg);
}

/*static*/ void InfoColl::newTreeHan(GroupOnlyMsg* msg) {
  return tree(msg);
}

InfoColl::ReducePtrType InfoColl::getReduce() const {
  return collective_->reduce_.get();
}

bool InfoColl::isReady() const {
  return
    in_phase_two_ && has_root_ &&
    (!is_in_group || collective_->span_ != nullptr);
}

void InfoColl::readyAction(ActionType const action) {
  if (isReady()) {
    action();
  } else {
    pending_ready_actions_.emplace_back(action);
  }
}

InfoColl::TreeType* InfoColl::getTree() const {
  vtAssert(collective_       , "Collective must exist");
  vtAssert(collective_->span_, "Spanning tree must exist");
  vtAssert(in_phase_two_     , "Must be in phase two");
  vtAssert(has_root_         , "Root node must be known by this node");
  vtAssert(
    known_root_node_ != uninitialized_destination, "Known root must be set"
  );
  return collective_->span_.get();
}

bool InfoColl::inGroup() const {
  return is_in_group;
}

}} /* end namespace vt::group */
