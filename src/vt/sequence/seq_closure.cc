
#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_closure.h"
#include "vt/sequence/seq_helpers.h"
#include "vt/sequence/seq_node.h"

namespace vt { namespace seq {

SeqClosure::SeqClosure(SeqNodePtrType in_child)
  : child_closure(in_child), is_leaf(false)
{ }

SeqClosure::SeqClosure(SeqLeafClosureType in_leaf)
  : leaf_closure(in_leaf), is_leaf(true)
{ }

SeqNodeStateEnumType SeqClosure::execute() {
  debug_print(
    sequence, node,
    "SeqClosure: execute is_leaf={}\n", print_bool(is_leaf)
  );

  if (is_leaf) {
    bool const should_block = leaf_closure();
    return should_block ?
      SeqNodeStateEnumType::WaitingNextState :
      SeqNodeStateEnumType::KeepExpandingState;
  } else {
    return child_closure->expandNext();
  }
}

}} //end namespace vt::seq
