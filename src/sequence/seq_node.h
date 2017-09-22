
#if ! defined __RUNTIME_TRANSPORT_SEQ_NODE__
#define __RUNTIME_TRANSPORT_SEQ_NODE__

#include <list>
#include <memory>
#include <cassert>
#include <cstdint>

#include "config.h"
#include "seq_common.h"

namespace vt { namespace seq {

enum class eSeqNodeType : int8_t {
  ParentNode = 1,
  LeafNode = 2,
  InvalidNode = -1
};

enum class eSeqNodeOrderType : int8_t {
  SequencedOrder = 3,
  ParallelOrder = 4,
  InvalidOrder = -2
};

enum class eSeqNodeState : int8_t {
  WaitingNextState = 1,
  KeepExpandingState = 2,
  InvalidState = -1
};

#define PRINT_SEQ_NODE_TYPE(NODE)                                       \
  ((NODE) == eSeqNodeType::ParentNode    ? "ParentNode"  :              \
   ((NODE) == eSeqNodeType::LeafNode     ? "LeafNode"    :              \
    ((NODE) == eSeqNodeType::InvalidNode ? "InvalidNode" : "???")       \
   )                                                                    \
  )

#define PRINT_SEQ_NODE_ORDER(NODE)                                      \
  ((NODE) == eSeqNodeOrderType::SequencedOrder ? "SequencedOrder" :     \
   ((NODE) == eSeqNodeOrderType::ParallelOrder ? "ParallelOrder" :      \
    ((NODE) == eSeqNodeOrderType::InvalidOrder ? "InvalidOrder" : "???") \
   )                                                                    \
  )

#define PRINT_SEQ_NODE_STATE(NODE)                                      \
  ((NODE) == eSeqNodeState::WaitingNextState ? "WaitingNextState" :     \
   ((NODE) == eSeqNodeState::KeepExpandingState ? "KeepExpandingState" : \
    ((NODE) == eSeqNodeState::InvalidState ? "InvalidState" : "???")    \
   )                                                                    \
  )

#define PRINT_SEQ_NODE_PTR(NODE) ((NODE) ? (NODE).get() : nullptr)

struct SeqNode;

template <typename T>
using SeqNodeContainerType = std::list<T>;
using SeqNodePtrType = std::shared_ptr<SeqNode>;
using SeqNodeEnumType = eSeqNodeType;
using SeqNodeOrderEnumType = eSeqNodeOrderType;
using SeqExpandFunType = std::function<bool()>;

union uSeqNodePayload {
  SeqNodeContainerType<SeqExpandFunType>* funcs;
  SeqNodeContainerType<SeqNodePtrType>* children;
};

static struct SeqNodeParentTag { } seq_node_parent_tag_t { };
static struct SeqNodeLeafTag { } seq_node_leaf_tag_t { };

struct SeqNode;

void setNode(SeqType const& id, SeqNodePtrType node);

struct SeqNode : std::enable_shared_from_this<SeqNode> {
  using SizeType = uint64_t;
  using SeqNodePayloadUnion = uSeqNodePayload;
  using SeqNodeStateEnumType = eSeqNodeState;
  using OrderEnum = SeqNodeOrderEnumType;
  using TypeEnum = SeqNodeEnumType;
  using WaitFunType = std::function<void()>;
  using WaitContainer = std::list<WaitFunType>;

  template <typename... Args>
  static SeqNodePtrType makeNode(
    SeqType const& id, SeqNodePtrType parent, Args&&... args
  ) {
    return std::make_shared<SeqNode>(id, parent, std::forward<Args...>(args...));
  }

  template <typename... Args>
  static SeqNodePtrType makeParallelNode(SeqType const& id, Args&&... args) {
    return std::make_shared<SeqNode>(
      id, seq_node_leaf_tag_t, std::forward<Args>(args)...
    );
  }

  SeqNode(SeqNodeParentTag, SeqType const& id)
    : order_type_(OrderEnum::SequencedOrder), type_(TypeEnum::ParentNode),
      seq_id(id) {
    payload_.children = new SeqNodeContainerType<SeqNodePtrType>{};

    debug_print(
      sequence, node,
      "SeqNode: creating with SeqNodeParentTag: (type=%s) (ptr=%p): "
      "children=%p, size=%ld\n",
      PRINT_SEQ_NODE_TYPE(type_), this,
      payload_.children, payload_.children->size()
    );
  }

  SeqNode(SeqNodeLeafTag, SeqType const& id)
    : order_type_(OrderEnum::SequencedOrder), type_(TypeEnum::LeafNode),
      seq_id(id) {
    payload_.funcs = new SeqNodeContainerType<SeqExpandFunType>{};

    debug_print(
      sequence, node,
      "SeqNode: creating with SeqNodeLeafTag: (type=%s) (ptr=%p): "
      "funcs=%p, size=%ld\n",
      PRINT_SEQ_NODE_TYPE(type_), this, payload_.funcs, payload_.funcs->size()
    );
  }

  SeqNode(SeqType const& id, SeqNodePtrType in_parent, SeqExpandFunType const& fn)
    : SeqNode(seq_node_leaf_tag_t, id) {
    parent_node_ = in_parent;
    addSequencedFunction(fn);
  }

  template <typename... FnT>
  SeqNode(SeqType const& id, SeqNodeLeafTag, FnT&&... fns)
    : SeqNode(seq_node_leaf_tag_t, id) {
    auto vec = {fns...};
    for (auto&& elm : vec) {
      payload_.funcs->push_back(elm);
    }
  }

  virtual ~SeqNode() {
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

  SizeType getSize() const {
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

  void setBlockedOnNode(eSeqConstructType cons, bool const& is_blocked) {
    blocked_on_node_ = is_blocked;
    ready_ = not is_blocked;

    debug_print(
      sequence, node,
      "SeqNode: (%p) blockedOnNode blocked_on_node_=%s\n",
      this, print_bool(blocked_on_node_)
    );
  }

  SeqNodeStateEnumType expandLeafNode() {
    debug_print(
      sequence, node,
      "SeqNode: expandLeafNode (%p): payload_.funcs: ptr=%p\n",
      this, payload_.funcs
    );

    if (payload_.funcs != nullptr) {
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

        if (funcs.size() > 0) {
          auto elm = funcs.front();
          funcs.pop_front();

          setNode(seq_id, this->shared_from_this());

          // run the function
          elm();

          setNode(seq_id, nullptr);

          debug_print(
            sequence, node,
            "SeqNode: (%p) after running elm (ready_=%s): blocked_on_node_=%s\n",
            this, print_bool(ready_), print_bool(blocked_on_node_)
          );

          if (blocked_on_node_) {
            cur_state = SeqNodeStateEnumType::WaitingNextState;
          }
        }
      } while (cur_state == SeqNodeStateEnumType::KeepExpandingState);

      return cur_state;
    } else {
      return SeqNodeStateEnumType::KeepExpandingState;
    }
  }

  SeqNodeStateEnumType expandParentNode() {
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

  void activate() {
    bool const has_sequenced_waits = sequenced_waits_.size() != 0;

    debug_print(
      sequence, node,
      "SeqNode: activate (%p) next=%s, parent=%s, blocked_on_node_=%s, "
      "has_sequenced_waits=%s\n",
      this, print_bool(next_node_), print_bool(parent_node_),
      print_bool(blocked_on_node_), print_bool(has_sequenced_waits)
    );

    assert(
      blocked_on_node_ == false and ready_ == true and "Must be ready, unblocked"
    );

    if (has_sequenced_waits) {
      auto fn = sequenced_waits_.front();
      sequenced_waits_.pop_front();
      fn();
    } else {
      auto const& next = next_node_ != nullptr ? next_node_ : parent_node_;
      if (next != nullptr) {
        next->expandNext();
      } else {
        debug_print(
          sequence, node,
          "SeqNode: activate (%p) done with sequence\n", this
        );
      }
    }
  }

  SeqNodeStateEnumType expandNext() {
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

  void addSequencedChild(SeqNodePtrType ptr) {
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

  void addSequencedFunction(SeqExpandFunType fun) {
    debug_print(
      sequence, node,
      "SeqNode: addSequencedFunction (%p) (type=%s): payload_.funcs.size=%ld\n",
      this, PRINT_SEQ_NODE_TYPE(type_), payload_.funcs->size()
    );

    assert(type_ == TypeEnum::LeafNode and "Must be leaf node");
    assert(order_type_ == OrderEnum::SequencedOrder and "Must be sequenced");

    payload_.funcs->push_back(fun);
  }

  void addParallelFunction(SeqExpandFunType fun) {
    debug_print(
      sequence, node,
      "SeqNode: addParallelFunction (%p) (type=%s): payload_.funcs.size=%ld\n",
      this, PRINT_SEQ_NODE_TYPE(type_), payload_.funcs->size()
    );

    assert(type_ == TypeEnum::LeafNode and "Must be leaf node");
    assert(order_type_ == OrderEnum::ParallelOrder and "Must be parallel");

    payload_.funcs->push_back(fun);
  }

  bool isReady() const {
    return ready_;
  }

  void setReady(bool const& ready) {
    ready_ = ready;
  }

  void setParent(SeqNodePtrType node) {
    parent_node_ = node;
  }

  void setNext(SeqNodePtrType node) {
    next_node_ = node;
  }

  bool isBlockedNode() const {
    return blocked_on_node_;
  }

  void addSequencedWait(WaitFunType fn) {
    sequenced_waits_.push_back(fn);

    debug_print(
      sequence, node,
      "SeqNode: addSequencedWait (%p) (type=%s): waits.size()=%ld\n",
      this, PRINT_SEQ_NODE_TYPE(type_), sequenced_waits_.size()
    );
  }

private:
  WaitContainer sequenced_waits_;

  bool ready_ = true;
  bool blocked_on_node_ = false;

  SeqNodePayloadUnion payload_;

  OrderEnum order_type_ = OrderEnum::InvalidOrder;

  TypeEnum type_ = TypeEnum::InvalidNode;

  SeqType seq_id = no_seq;

  SeqNodePtrType parent_node_ = nullptr;
  SeqNodePtrType next_node_ = nullptr;
};

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_NODE__*/
