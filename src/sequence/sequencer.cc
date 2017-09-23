
#include "config.h"
#include "function.h"

#include "seq_common.h"
#include "sequencer.h"

namespace vt { namespace seq {

bool contextualExecution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
) {
  theSeq->lookupContextExecute(seq, std::forward<SeqCallableType>(callable));
  return true;
}

SeqNodeStateEnumType SeqClosure::execute() {
  debug_print(
    sequence, node,
    "SeqClosure: execute is_leaf=%s\n", print_bool(is_leaf)
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


// template <typename SeqTag, template <typename> class SeqTrigger>
// /*static*/
// typename TaggedSequencer<SeqTag, SeqTrigger>::SeqFunType
// TaggedSequencer<SeqTag, SeqTrigger>::convertSeqFun(
//   SeqType const& id, UserSeqFunType fn
// ) {
//   return [=]() -> bool { return executeInContext(id, true, fn); };
// }


}} //end namespace vt::seq
