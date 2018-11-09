
#if !defined INCLUDED_SEQUENCE_SEQ_CONTEXT_H
#define INCLUDED_SEQUENCE_SEQ_CONTEXT_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_node.h"
#include "vt/sequence/seq_ult_context.h"

#include <list>
#include <memory>

#include <context/context_wrapper.h>

namespace vt { namespace seq {

struct SeqContext {
  using SeqContextULTType = SeqULTContext;
  using SeqContextULTPtrType = std::unique_ptr<SeqULTContext>;

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
  void suspend();
  void resume();

  SeqContextULTPtrType seq_ult = nullptr;

private:
  bool suspendable_ = false;

  SeqNodePtrType node_ = nullptr;

  SeqType seq_id = no_seq;
};

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_CONTEXT_H*/
