
#if ! defined __RUNTIME_TRANSPORT_SEQ_LIST__
#define __RUNTIME_TRANSPORT_SEQ_LIST__

#include <list>

#include "config.h"
#include "seq_common.h"
#include "seq_node.h"

namespace vt { namespace seq {

struct SeqList {
  using SeqFunType = SystemSeqFunType;
  using SeqNodeType = SeqNode;
  using SeqNodeStateEnumType = eSeqNodeState;

  explicit SeqList(SeqType const& seq_id_in)
    : seq_id_(seq_id_in), ready_(true),
      node_(new SeqNode(seq_node_parent_tag_t, seq_id_))
  { }

  void addAction(SeqFunType const& fn) {
    debug_print(
      sequence, node,
      "SeqList: addAction id=%d, node_=%p\n", seq_id_, PRINT_SEQ_NODE_PTR(node_)
    );

    node_->addSequencedChild(SeqNodeType::makeNode(seq_id_, node_, fn));
  }

  void addNode(SeqNodePtrType node) {
    debug_print(
      sequence, node,
      "SeqList: addNode id=%d, node_=%p\n", seq_id_, PRINT_SEQ_NODE_PTR(node_)
    );

    node_->addSequencedChild(std::move(node));
  }

  void expandNextNode() {
    ready_ = false;

    auto const& state = node_->expandNext();

    debug_print(
      sequence, node,
      "SeqList: expandNextNode id=%d, node_=%p, state=%s\n",
      seq_id_, PRINT_SEQ_NODE_PTR(node_), PRINT_SEQ_NODE_STATE(state)
    );

    switch (state) {
    case SeqNodeStateEnumType::WaitingNextState:
      return;
      break;
    case SeqNodeStateEnumType::KeepExpandingState:
      expandNextNode();
      break;
    default:
      assert(0 and "This should never happen");
    }
  }

  void makeReady() {
    ready_ = true;
  }

  bool isReady() const {
    return ready_;
  }

  SeqType getSeq() const {
    return seq_id_;
  }

private:
  SeqType seq_id_ = no_seq;

  bool ready_ = true;

  SeqNodePtrType node_ = nullptr;
};

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_LIST__*/
