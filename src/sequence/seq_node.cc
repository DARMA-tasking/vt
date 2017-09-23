
#include "config.h"
#include "seq_common.h"
#include "seq_helpers.h"
#include "seq_closure.h"
#include "seq_node.h"
#include "sequencer.h"

namespace vt { namespace seq {

SeqNode::SeqNode(SeqNodeParentTag, SeqType const& id)
  : order_type_(OrderEnum::SequencedOrder), type_(TypeEnum::ParentNode),
    seq_id_(id)
{
  payload_.children = new SeqNodeContainerType<SeqNodePtrType>{};

  debug_print(
    sequence, node,
    "SeqNode: creating with SeqNodeParentTag: (type=%s) (ptr=%p): "
    "children=%p, size=%ld\n",
    PRINT_SEQ_NODE_TYPE(type_), this,
    payload_.children, payload_.children->size()
  );
}

SeqNode::SeqNode(SeqNodeLeafTag, SeqType const& id)
  : order_type_(OrderEnum::SequencedOrder), type_(TypeEnum::LeafNode),
    seq_id_(id)
{
  payload_.funcs = new SeqNodeContainerType<SeqExpandFunType>{};

  debug_print(
    sequence, node,
    "SeqNode: creating with SeqNodeLeafTag: (type=%s) (ptr=%p): "
    "funcs=%p, size=%ld\n",
    PRINT_SEQ_NODE_TYPE(type_), this, payload_.funcs, payload_.funcs->size()
  );
}

SeqNode::SeqNode(
  SeqType const& id, SeqNodePtrType parent, SeqExpandFunType const& fn
) : SeqNode(seq_node_leaf_tag_t, id)
{
  parent_node_ = parent;
  addSequencedFunction(fn);
}

/*virtual*/ SeqNode::~SeqNode() {
  switch (type_) {
  case TypeEnum::LeafNode:
    delete payload_.funcs;
    break;
  case TypeEnum::ParentNode:
    delete payload_.children;
    break;
  case TypeEnum::InvalidNode:
    // do nothing
    break;
  default:
    assert(0 and "Should never be able to reach this case");
  }

  // set to invalid just for sanity's sake
  type_ = TypeEnum::InvalidNode;
}

SeqNode::SizeType SeqNode::getSize() const {
  switch (type_) {
  case TypeEnum::LeafNode:
    return payload_.funcs ? payload_.funcs->size() : 0;
    break;
  case TypeEnum::ParentNode:
    return payload_.children ? payload_.children->size() : 0;
    break;
  case TypeEnum::InvalidNode:
    return 0;
  }
}

void SeqNode::setBlockedOnNode(eSeqConstructType cons, bool const& is_blocked) {
  blocked_on_node_ = is_blocked;
  ready_ = not is_blocked;

  debug_print(
    sequence, node,
    "SeqNode: (%p) blockedOnNode blocked_on_node_=%s\n",
    this, print_bool(blocked_on_node_)
  );
}

SeqNodeStateEnumType SeqNode::expandLeafNode() {
  debug_print(
    sequence, node,
    "SeqNode: expandLeafNode (%p): payload_.funcs: ptr=%p\n",
    this, payload_.funcs
  );

  if (payload_.funcs != nullptr) {
    auto this_node = this->shared_from_this();
    SeqNodeStateEnumType cur_state = SeqNodeStateEnumType::KeepExpandingState;

    auto& funcs = *payload_.funcs;
    do {
      debug_print(
        sequence, node,
        "SeqNode: expandLeafNode (%p) (type=%s): funcs.size()=%ld, "
        "cur_state=%s\n",
        this, PRINT_SEQ_NODE_TYPE(type_), funcs.size(),
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
        "SeqNode: (%p) after running elm (ready_=%s): blocked_on_node_=%s\n",
        this, print_bool(ready_), print_bool(blocked_on_node_)
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
    "SeqNode: expandParentNode (%p) (type=%s): payload_.children: ptr=%p\n",
    this, PRINT_SEQ_NODE_TYPE(type_), payload_.children
  );

  SeqNodeStateEnumType cur_state = SeqNodeStateEnumType::KeepExpandingState;

  if (payload_.children != nullptr) {
    auto& children = *payload_.children;
    do {
      debug_print(
        sequence, node,
        "SeqNode: expandParentNode (%p) (type=%s): children.size()=%ld\n",
        this, PRINT_SEQ_NODE_TYPE(type_), children.size()
      );

      if (children.size() > 0) {
        auto child = std::move(children.front());
        children.pop_front();
        cur_state = child->expandNext();
      } else {
        break;
      }
    } while (cur_state == SeqNodeStateEnumType::KeepExpandingState);
  }

  return cur_state;
}

void SeqNode::executeClosuresUntilBlocked() {
  debug_print(
    sequence, node,
    "SeqNode: executeClosuresUntilBlocked (%p): num=%ld: blocked=%s\n",
    this, sequenced_closures_.size(), print_bool(blocked_on_node_)
  );

  do {
    if (sequenced_closures_.size() != 0) {
      debug_print(
        sequence, node,
        "SeqNode: executeClosuresUntilBlocked (%p) execute closure: num=%ld\n",
        this, sequenced_closures_.size()
      );

      auto closure = sequenced_closures_.front();
      sequenced_closures_.pop_front();
      auto const& status = closure.execute();

      if (status == SeqNodeStateEnumType::WaitingNextState or blocked_on_node_) {
        break;
      }
    }
  } while (sequenced_closures_.size() != 0);
}

void SeqNode::activate() {
  auto const& unexpanded_size = this->getSize();
  bool const has_unexpanded_nodes = unexpanded_size != 0;
  bool const has_sequenced_closures = sequenced_closures_.size() != 0;

  debug_print(
    sequence, node,
    "SeqNode: activate (%p) next=%s, parent=%s, blocked=%s, "
    "has_sequenced_closures=%s, num closures=%ld, num unexpanded nodes=%lld\n",
    this, print_bool(next_node_), print_bool(parent_node_),
    print_bool(blocked_on_node_), print_bool(has_sequenced_closures),
    sequenced_closures_.size(), unexpanded_size
  );

  assert(
    blocked_on_node_ == false and ready_ == true and "Must be ready+unblocked"
  );

  if (has_sequenced_closures) {
    // Wait on all sequenced closures that have not been executed
    executeClosuresUntilBlocked();
  } else if (has_unexpanded_nodes) {
    // If all sequenced closures are executed, expand the next node
    expandNext();
  } else {
    // This node is finished, execute the next sibling or the parent
    auto const& next = next_node_ != nullptr ? next_node_ : parent_node_;

    debug_print(
      sequence, node,
      "SeqNode: activate (%p) next=%p, parent=%p, selected=%p\n",
      this, PRINT_SEQ_NODE_PTR(next_node_), PRINT_SEQ_NODE_PTR(parent_node_),
      PRINT_SEQ_NODE_PTR(next)
    );

    if (next != nullptr) {
      // @todo: is this correct?
      next->setBlockedOnNode(eSeqConstructType::WaitConstruct, false);
      next->activate();
    } else {
      debug_print(
        sequence, node,
        "SeqNode: activate (%p) done with sequence\n", this
      );
    }
  }
}

SeqNodeStateEnumType SeqNode::expandNext() {
  debug_print(
    sequence, node,
    "SeqNode: expandNext (%p) (type=%s)\n",
    this, PRINT_SEQ_NODE_TYPE(type_)
  );

  switch (type_) {
  case TypeEnum::LeafNode:
    return expandLeafNode();
    break;
  case TypeEnum::ParentNode:
    return expandParentNode();
    break;
  case TypeEnum::InvalidNode:
    return SeqNodeStateEnumType::InvalidState;
    break;
  default:
    assert(0 and "This should never happen");
  }

  return SeqNodeStateEnumType::InvalidState;
}

void SeqNode::addSequencedChild(SeqNodePtrType ptr) {
  debug_print(
    sequence, node,
    "SeqNode: addSequencedChild (%p) (type=%s): ptr=%p: "
    "payload_.children.size=%ld\n",
    this, PRINT_SEQ_NODE_TYPE(type_), ptr.get(), payload_.children->size()
  );

  assert(type_ == TypeEnum::ParentNode and "Must be parent node");
  assert(order_type_ == OrderEnum::SequencedOrder and "Must be sequenced");

  if (payload_.children->size() > 0) {
    auto const& cur = payload_.children->begin();
    ptr->setNext(*cur);
  }

  payload_.children->push_back(std::move(ptr));
}

void SeqNode::addSequencedFunction(SeqExpandFunType fun) {
  debug_print(
    sequence, node,
    "SeqNode: addSequencedFunction (%p) (type=%s): payload_.funcs.size=%ld\n",
    this, PRINT_SEQ_NODE_TYPE(type_), payload_.funcs->size()
  );

  assert(type_ == TypeEnum::LeafNode and "Must be leaf node");
  assert(order_type_ == OrderEnum::SequencedOrder and "Must be sequenced");

  payload_.funcs->push_back(fun);
}

void SeqNode::addParallelFunction(SeqExpandFunType fun) {
  debug_print(
    sequence, node,
    "SeqNode: addParallelFunction (%p) (type=%s): payload_.funcs.size=%ld\n",
    this, PRINT_SEQ_NODE_TYPE(type_), payload_.funcs->size()
  );

  assert(type_ == TypeEnum::LeafNode and "Must be leaf node");
  assert(order_type_ == OrderEnum::ParallelOrder and "Must be parallel");

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

void SeqNode::addSequencedClosure(SeqLeafClosureType cl, bool const& is_leaf) {
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
    "SeqNode: addSequencedClosure (%p) (type=%s): num closures=%ld\n",
    this, PRINT_SEQ_NODE_TYPE(type_), sequenced_closures_.size()
  );

  bool const is_ready = not isBlockedNode();
  if (is_ready) {
    executeClosuresUntilBlocked();
  }
}

SeqType SeqNode::getSeqID() const {
  return seq_id_;
}

}} //end namespace vt::seq
