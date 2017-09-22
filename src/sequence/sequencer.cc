
#include "config.h"
#include "function.h"

#include "seq_common.h"
#include "sequencer.h"

namespace vt { namespace seq {

bool contextualExecution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
) {
  return theSeq->executeInContext(
    seq, is_sequenced, std::forward<SeqCallableType>(callable)
  );
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
