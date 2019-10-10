/*
//@HEADER
// *****************************************************************************
//
//                                 seq_node.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_helpers.h"
#include "vt/sequence/seq_closure.h"
#include "vt/sequence/seq_node.h"
#include "vt/sequence/sequencer.h"

namespace vt { namespace seq {

SeqNode::SeqNode(SeqNodeParentTag, SeqType const& id)
  : SeqNode(seq_node_universal_tag_t, id, OrderEnum::SequencedOrder,
            TypeEnum::ParentNode)
{
  payload_.children = new SeqNodeContainerType<SeqNodePtrType>{};

  debug_print(
    sequence, node,
    "SeqNode: creating with SeqNodeParentTag: (type={}) (ptr={}): "
    "children={}, size={}\n",
    PRINT_SEQ_NODE_TYPE(type_), print_ptr(this),
    print_ptr(payload_.children), payload_.children->size()
  );
}

SeqNode::SeqNode(SeqNodeLeafTag, SeqType const& id)
  : SeqNode(seq_node_universal_tag_t, id, OrderEnum::SequencedOrder,
            TypeEnum::LeafNode)
{
  payload_.funcs = new SeqNodeContainerType<SeqExpandFunType>{};

  debug_print(
    sequence, node,
    "SeqNode: creating with SeqNodeLeafTag: (type={}) (ptr={}): "
    "funcs={}, size={}\n",
    PRINT_SEQ_NODE_TYPE(type_), print_ptr(this), print_ptr(payload_.funcs),
    payload_.funcs->size()
  );
}

SeqNode::SeqNode(SeqNodeParallelTag, SeqType const& id, SeqParallelPtrType par)
  : SeqNode(seq_node_universal_tag_t, id, OrderEnum::ParallelOrder,
            TypeEnum::ParallelNode)
{
  payload_.parallel = par;

  debug_print(
    sequence, node,
    "SeqNode: creating with SeqNodeParallelTag: (type={}) (ptr={}): "
    "parallel={}\n",
    PRINT_SEQ_NODE_TYPE(type_), print_ptr(this), print_ptr(payload_.parallel)
  );
}

SeqNode::SeqNode(
  SeqType const& id, SeqNodePtrType parent, SeqExpandFunType const& fn
) : SeqNode(seq_node_leaf_tag_t, id)
{
  parent_node_ = parent;
  addSequencedFunction(fn);
}

SeqNode::SeqNode(
  SeqNodeUniversalTag, SeqType const& id, OrderEnum const& order,
  TypeEnum const& type
) : order_type_(order), type_(type), seq_id_(id)
{
}

/*virtual*/ SeqNode::~SeqNode() {
  switch (type_) {
  case TypeEnum::LeafNode:
    delete payload_.funcs;
    break;
  case TypeEnum::ParentNode:
    delete payload_.children;
    break;
  case TypeEnum::ParallelNode:
    delete payload_.parallel;
    break;
  case TypeEnum::InvalidNode:
    // do nothing
    break;
  default:
    vtAssert(0, "Should never be able to reach this case");
  }

  // set to invalid just for sanity's sake
  type_ = TypeEnum::InvalidNode;
}

SeqNode::SizeType SeqNode::getSize() const {
  switch (type_) {
  case TypeEnum::LeafNode:
    return payload_.funcs ? payload_.funcs->size() : 0;
  case TypeEnum::ParentNode:
    return payload_.children ? payload_.children->size() : 0;
  case TypeEnum::ParallelNode:
    return payload_.parallel ? payload_.parallel->getSize() : 0;
  case TypeEnum::InvalidNode:
    return 0;
  default:
    vtAssert(0, "Should not be reachable");
    return 0;
  }
}

void SeqNode::setBlockedOnNode(eSeqConstructType cons, bool const& is_blocked) {
  if (type_ != TypeEnum::ParallelNode) {
    blocked_on_node_ = is_blocked;
    ready_ = not is_blocked;

    debug_print(
      sequence, node,
      "SeqNode: ({}) blockedOnNode blocked_on_node_={}\n",
      print_ptr(this), print_bool(blocked_on_node_)
    );
  }
}

SeqNodeStateEnumType SeqNode::expandLeafNode() {
  debug_print(
    sequence, node,
    "SeqNode: expandLeafNode ({}): payload_.funcs: ptr={}\n",
    print_ptr(this), print_ptr(payload_.funcs)
  );

  if (payload_.funcs != nullptr) {
    auto this_node = this->shared_from_this();
    SeqNodeStateEnumType cur_state = SeqNodeStateEnumType::KeepExpandingState;

    auto& funcs = *payload_.funcs;
    do {
      debug_print(
        sequence, node,
        "SeqNode: expandLeafNode ({}) (type={}): funcs.size()={}, "
        "cur_state={}\n",
        print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), funcs.size(),
        PRINT_SEQ_NODE_STATE(cur_state)
      );

      if (funcs.size() == 0) {
        break;
      }

      auto elm = funcs.front();
      funcs.pop_front();

      // run the function in this node context
      executeSeqExpandContext(seq_id_, this_node, elm);

      debug_print(
        sequence, node,
        "SeqNode: ({}) after running elm (ready_={}): blocked_on_node_={}\n",
        print_ptr(this), print_bool(ready_), print_bool(blocked_on_node_)
      );

      if (blocked_on_node_) {
        cur_state = SeqNodeStateEnumType::WaitingNextState;
      }
    } while (cur_state == SeqNodeStateEnumType::KeepExpandingState);

    return cur_state;
  } else {
    return SeqNodeStateEnumType::KeepExpandingState;
  }
}

SeqNodeStateEnumType SeqNode::expandParentNode() {
  debug_print(
    sequence, node,
    "SeqNode: expandParentNode ({}) (type={}): payload_.children: ptr={}\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), print_ptr(payload_.children)
  );

  SeqNodeStateEnumType cur_state = SeqNodeStateEnumType::KeepExpandingState;

  if (payload_.children != nullptr) {
    auto& children = *payload_.children;
    do {
      debug_print(
        sequence, node,
        "SeqNode: expandParentNode ({}) (type={}): children.size()={}\n",
        print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), children.size()
      );

      if (children.size() > 0) {
        auto child = std::move(children.front());
        children.pop_front();
        cur_state = child->expandNext();
      } else {
        cur_state = SeqNodeStateEnumType::NoMoreExpansionsState;
        break;
      }
    } while (cur_state == SeqNodeStateEnumType::KeepExpandingState);
  }

  return cur_state;
}

bool SeqNode::executeClosuresUntilBlocked() {
  debug_print(
    sequence, node,
    "SeqNode: executeClosuresUntilBlocked (begin) ({}): num={}: blocked={}\n",
    print_ptr(this), sequenced_closures_.size(), print_bool(blocked_on_node_)
  );

  bool blocked = false;

  do {
    if (sequenced_closures_.size() != 0) {
      debug_print(
        sequence, node,
        "SeqNode: executeClosuresUntilBlocked ({}) execute closure: num={}\n",
        print_ptr(this), sequenced_closures_.size()
      );

      // @todo: implement deferred execution here
      // auto execute_ready_closure = [=]{
      //   closure.execute();
      // };
      // theSeq()->enqueue(execute_ready_closure);

      auto closure = sequenced_closures_.front();
      sequenced_closures_.pop_front();
      auto const& status = closure.execute();

      bool const is_waiting = status == SeqNodeStateEnumType::WaitingNextState;
      blocked = is_waiting or blocked_on_node_;

      debug_print(
        sequence, node,
        "SeqNode: executeClosuresUntilBlocked ({}) execute closure: status={}, "
        "is_waiting={}, blocked={}\n",
        print_ptr(this), PRINT_SEQ_NODE_STATE(status), print_bool(is_waiting),
        print_bool(blocked)
      );
    }
  } while (sequenced_closures_.size() != 0 and not blocked);


  if (blocked) {
    setBlockedOnNode(eSeqConstructType::WaitConstruct, blocked);
  }

  debug_print(
    sequence, node,
    "SeqNode: executeClosuresUntilBlocked (end) ({}): num={}: "
    "blocked_on_node_={}, blocked={}\n",
    print_ptr(this), sequenced_closures_.size(), print_bool(blocked_on_node_),
    print_bool(blocked)
  );

  return blocked;
}

void SeqNode::activate() {
  bool const type_is_parallel = type_ == TypeEnum::ParallelNode;
  bool finished_parallel = false;

  debug_print(
    sequence, node,
    "SeqNode: activate ({}), type={}, type_is_parallel={}\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), print_bool(type_is_parallel)
  );

  if (type_is_parallel) {
    finished_parallel = payload_.parallel->join();

    debug_print(
      sequence, node,
      "SeqNode: activate finished__parallel={}\n", print_bool(finished_parallel)
    );
  }

  if (not type_is_parallel or finished_parallel) {
    auto const& unexpanded_size = this->getSize();
    bool const has_unexpanded_nodes = unexpanded_size != 0;
    bool const has_sequenced_closures = sequenced_closures_.size() != 0;

    debug_print(
      sequence, node,
      "SeqNode: activate ({}) next={}, parent={}, blocked={}, "
      "has_sequenced_closures={}, num closures={}, num unexpanded nodes={}\n",
      print_ptr(this), print_bool(next_node_), print_bool(parent_node_),
      print_bool(blocked_on_node_), print_bool(has_sequenced_closures),
      sequenced_closures_.size(), unexpanded_size
    );

    vtAssert(
      blocked_on_node_ == false and ready_ == true, "Must be ready+unblocked"
    );

    bool blocked = false;
    // Wait on all sequenced closures that have not been executed
    if (has_sequenced_closures) {
      blocked = executeClosuresUntilBlocked();
    }

    SeqNodeStateEnumType state_ret = SeqNodeStateEnumType::InvalidState;

    // If all sequenced closures are executed, expand the next node
    if (not blocked and has_unexpanded_nodes) {
      state_ret = expandNext();
    }

    bool const expanded_all_nodes =
      not has_unexpanded_nodes or
      state_ret == SeqNodeStateEnumType::KeepExpandingState;

    if (not blocked and expanded_all_nodes) {
      // This node is finished, execute the next sibling or the parent
      auto const& next = next_node_ != nullptr ? next_node_ : parent_node_;

      debug_print(
        sequence, node,
        "SeqNode: activate ({}) next={}, parent={}, selected={}\n",
        print_ptr(this), PRINT_SEQ_NODE_PTR(next_node_), PRINT_SEQ_NODE_PTR(parent_node_),
        PRINT_SEQ_NODE_PTR(next)
      );

      if (next != nullptr) {
        // @todo: is this correct?
        next->setBlockedOnNode(eSeqConstructType::WaitConstruct, false);
        next->activate();
      } else {
        debug_print(
          sequence, node,
          "SeqNode: activate ({}) done with sequence\n", print_ptr(this)
        );
      }
    }
  }
}

SeqNodeStateEnumType SeqNode::expandNext() {
  debug_print(
    sequence, node,
    "SeqNode: expandNext ({}) (type={})\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_)
  );

  switch (type_) {
  case TypeEnum::LeafNode:
    return expandLeafNode();
  case TypeEnum::ParentNode:
    return expandParentNode();
  case TypeEnum::ParallelNode:
    return payload_.parallel->expandParallelNode(this->shared_from_this());
  case TypeEnum::InvalidNode:
    return SeqNodeStateEnumType::InvalidState;
  default:
    vtAssert(0, "This should never happen");
  }

  return SeqNodeStateEnumType::InvalidState;
}

void SeqNode::addSequencedChild(SeqNodePtrType ptr) {
  debug_print(
    sequence, node,
    "SeqNode: addSequencedChild ({}) (type={}): ptr={}: "
    "payload_.children.size={}\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), print_ptr(ptr.get()),
    payload_.children->size()
  );

  vtAssert(type_ == TypeEnum::ParentNode, "Must be parent node");
  vtAssert(order_type_ == OrderEnum::SequencedOrder, "Must be sequenced");

  if (payload_.children->size() > 0) {
    auto const& cur = payload_.children->begin();
    ptr->setNext(*cur);
  }

  payload_.children->push_back(std::move(ptr));
}

void SeqNode::addSequencedFunction(SeqExpandFunType fun) {
  debug_print(
    sequence, node,
    "SeqNode: addSequencedFunction ({}) (type={}): payload_.funcs.size={}\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), payload_.funcs->size()
  );

  vtAssert(type_ == TypeEnum::LeafNode, "Must be leaf node");
  vtAssert(order_type_ == OrderEnum::SequencedOrder, "Must be sequenced");

  payload_.funcs->push_back(fun);
}

void SeqNode::addParallelFunction(SeqExpandFunType fun) {
  debug_print(
    sequence, node,
    "SeqNode: addParallelFunction ({}) (type={}): payload_.funcs.size={}\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), payload_.funcs->size()
  );

  vtAssert(type_ == TypeEnum::LeafNode, "Must be leaf node");
  vtAssert(order_type_ == OrderEnum::ParallelOrder, "Must be parallel");

  payload_.funcs->push_back(fun);
}

bool SeqNode::isReady() const {
  return ready_;
}

void SeqNode::setReady(bool const& ready) {
  ready_ = ready;
}

void SeqNode::setParent(SeqNodePtrType node) {
  parent_node_ = node;
}

void SeqNode::setNext(SeqNodePtrType node) {
  next_node_ = node;
}

bool SeqNode::isBlockedNode() const {
  return blocked_on_node_;
}

void SeqNode::addSequencedParallelClosure(SeqNodePtrType par_node) {
  sequenced_closures_.emplace_back(SeqClosure{par_node});

  debug_print(
    sequence, node,
    "SeqNode: addSequencedParallelClosure ({}) (type={}): num closures={}\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), sequenced_closures_.size()
  );

  executeIfReady();
}

void SeqNode::addSequencedClosure(SeqLeafClosureType cl, bool const& is_leaf) {
  debug_print(
    sequence, node,
    "SeqNode: addSequencedClosure ({}) (type={}): num closures={}, is_leaf={}\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), sequenced_closures_.size(),
    print_bool(is_leaf)
  );

  if (is_leaf) {
    sequenced_closures_.push_back(SeqClosure{cl});
  } else {
    auto node = this->shared_from_this();
    sequenced_closures_.emplace_back(
      SeqClosure{SeqNode::makeNode(seq_id_, node, cl)}
    );
  }

  debug_print(
    sequence, node,
    "SeqNode: addSequencedClosure ({}) (type={}): num closures={}\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), sequenced_closures_.size()
  );

  executeIfReady();
}

void SeqNode::executeIfReady() {
  bool const is_ready = not isBlockedNode();

  debug_print(
    sequence, node,
    "SeqNode: executeIfReady ({}) (type={}): num closures={}: is_ready={}\n",
    print_ptr(this), PRINT_SEQ_NODE_TYPE(type_), sequenced_closures_.size(),
    print_bool(is_ready)
  );

  if (is_ready) {
    executeClosuresUntilBlocked();
  }
}

SeqType SeqNode::getSeqID() const {
  return seq_id_;
}

}} //end namespace vt::seq
