#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_list.h"

namespace vt { namespace seq {

/*explicit*/ SeqList::SeqList(SeqType const& seq_id_in)
  : seq_id_(seq_id_in), ready_(true),
    node_(new SeqNode(seq_node_parent_tag_t, seq_id_))
{ }

void SeqList::addAction(SeqFunType const& fn) {
  debug_print(
    sequence, node,
    "SeqList: addAction id={}, node_={}\n", seq_id_, PRINT_SEQ_NODE_PTR(node_)
  );

  node_->addSequencedChild(SeqNodeType::makeNode(seq_id_, node_, fn));
}

void SeqList::addNode(SeqNodePtrType node) {
  debug_print(
    sequence, node,
    "SeqList: addNode id={}, node_={}\n", seq_id_, PRINT_SEQ_NODE_PTR(node_)
  );

  node_->addSequencedChild(std::move(node));
}

void SeqList::expandNextNode() {
  ready_ = false;

  auto const& state = node_->expandNext();

  debug_print(
    sequence, node,
    "SeqList: expandNextNode id={}, node_={}, state={}\n",
    seq_id_, PRINT_SEQ_NODE_PTR(node_), PRINT_SEQ_NODE_STATE(state)
  );

  switch (state) {
  case SeqNodeStateEnumType::WaitingNextState:
  case SeqNodeStateEnumType::NoMoreExpansionsState:
    return;
    break;
  case SeqNodeStateEnumType::KeepExpandingState:
    expandNextNode();
    break;
  default:
    vtAssert(0, "This should never happen");
  }
}

void SeqList::makeReady() {
  ready_ = true;
}

bool SeqList::isReady() const {
  return ready_;
}

SeqType SeqList::getSeq() const {
  return seq_id_;
}

}} //end namespace vt::seq
