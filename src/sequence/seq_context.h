
#if ! defined __RUNTIME_TRANSPORT_SEQ_CONTEXT__
#define __RUNTIME_TRANSPORT_SEQ_CONTEXT__

#include <list>
#include <cassert>

#include "config.h"
#include "seq_common.h"
#include "seq_node.h"
#include "seq_ult_context.h"

namespace vt { namespace seq {

struct SeqContext {
  SeqContext(
    SeqType const& in_seq_id, SeqNodePtrType in_node, bool is_suspendable = false
  );
  SeqContext(SeqType const& in_seq_id);
  SeqContext(SeqContext const&) = delete;

  SeqType getSeq() const;
  SeqNodePtrType getNode() const;
  void setNode(SeqNodePtrType node);
  bool isSuspendable() const;
  void setSuspendable(bool const is_suspendable);

private:
  bool suspendable_ = false;

  SeqNodePtrType node_ = nullptr;

  SeqType seq_id = no_seq;
};

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_CONTEXT__*/
