
#if ! defined __RUNTIME_TRANSPORT_SEQ_CONTEXT__
#define __RUNTIME_TRANSPORT_SEQ_CONTEXT__

#include <list>
#include <cassert>

#include "config.h"
#include "seq_common.h"
#include "seq_node.h"

namespace vt { namespace seq {

struct SeqContext {
  SeqContext(SeqType const& in_seq_id, SeqNodePtrType in_node)
    : node_(in_node), seq_id(in_seq_id) {
    debug_print(
      sequence, node,
      "SeqContext: construct: node=%p, id=%d\n", PRINT_SEQ_NODE_PTR(node_), seq_id
    );
  }
  SeqContext(SeqType const& in_seq_id) : seq_id(in_seq_id) { }
  SeqContext(SeqContext const&) = delete;

  SeqType getSeq() const {
    return seq_id;
  }

  SeqNodePtrType getNode() const {
    return node_;
  }

  void setNode(SeqNodePtrType node) {
    debug_print(
      sequence, node,
      "SeqContext: setNode: node=%p, id=%d\n", PRINT_SEQ_NODE_PTR(node), seq_id
    );

    node_ = node;
  }

private:
  SeqNodePtrType node_ = nullptr;

  SeqType seq_id = no_seq;
};

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_CONTEXT__*/
