
#if !defined INCLUDED_SEQUENCE_SEQ_CLOSURE_H
#define INCLUDED_SEQUENCE_SEQ_CLOSURE_H

#include <list>
#include <memory>
#include <cassert>
#include <cstdint>

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_helpers.h"

namespace vt { namespace seq {

struct SeqClosure {
  SeqNodePtrType child_closure = nullptr;
  SeqLeafClosureType leaf_closure = nullptr;
  bool is_leaf = false;

  explicit SeqClosure(SeqNodePtrType in_child);
  explicit SeqClosure(SeqLeafClosureType in_leaf);

  SeqNodeStateEnumType execute();
};

using SeqExpandedClosureType = SeqClosure;

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_CLOSURE_H*/
