
#if ! defined __RUNTIME_TRANSPORT_SEQ_CLOSURE__
#define __RUNTIME_TRANSPORT_SEQ_CLOSURE__

#include <list>
#include <memory>
#include <cassert>
#include <cstdint>

#include "config.h"
#include "seq_common.h"
#include "seq_helpers.h"

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

#endif /* __RUNTIME_TRANSPORT_SEQ_CLOSURE__*/
