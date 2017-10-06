
#include "config.h"
#include "seq_common.h"
#include "seq_node.h"
#include "seq_context.h"

namespace vt { namespace seq {

SeqContext::SeqContext(
  SeqType const& in_seq_id, SeqNodePtrType in_node, bool is_suspendable
) : suspendable_(is_suspendable), node_(in_node), seq_id(in_seq_id)
{
  debug_print(
    sequence, node,
    "SeqContext: construct: node=%p, id=%d, suspendable=%s\n",
    PRINT_SEQ_NODE_PTR(node_), seq_id, print_bool(suspendable_)
  );
}
SeqContext::SeqContext(SeqType const& in_seq_id) : seq_id(in_seq_id) { }

SeqType SeqContext::getSeq() const {
  return seq_id;
}

SeqNodePtrType SeqContext::getNode() const {
  return node_;
}

void SeqContext::setNode(SeqNodePtrType node) {
  debug_print(
    sequence, node,
    "SeqContext: setNode: node=%p, id=%d\n", PRINT_SEQ_NODE_PTR(node), seq_id
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
