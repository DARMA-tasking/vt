
#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_node.h"
#include "vt/sequence/seq_context.h"
#include "vt/sequence/seq_ult_context.h"

#include <cassert>

namespace vt { namespace seq {

SeqContext::SeqContext(
  SeqType const& in_seq_id, SeqNodePtrType in_node, bool is_suspendable
) : suspendable_(is_suspendable), node_(in_node), seq_id(in_seq_id)
{
  debug_print(
    sequence, node,
    "SeqContext: construct: node={}, id={}, suspendable={}\n",
    PRINT_SEQ_NODE_PTR(node_), seq_id, print_bool(suspendable_)
  );

  if (suspendable_) {
    seq_ult = std::make_unique<SeqContextULTType>();
  }
}
SeqContext::SeqContext(SeqType const& in_seq_id) : seq_id(in_seq_id) { }

void SeqContext::suspend() {
  assert(seq_ult != nullptr and "Seq ULT must be live");
  seq_ult->suspend();
}

void SeqContext::resume() {
  assert(seq_ult != nullptr and "Seq ULT must be live");
  seq_ult->resume();
}

SeqType SeqContext::getSeq() const {
  return seq_id;
}

SeqNodePtrType SeqContext::getNode() const {
  return node_;
}

void SeqContext::setNode(SeqNodePtrType node) {
  debug_print(
    sequence, node,
    "SeqContext: setNode: node={}, id={}\n", PRINT_SEQ_NODE_PTR(node), seq_id
  );

  node_ = node;
}

bool SeqContext::isSuspendable() const {
  return suspendable_;
}

void SeqContext::setSuspendable(bool const is_suspendable) {
  suspendable_ = is_suspendable;
}

}} //end namespace vt::seq
