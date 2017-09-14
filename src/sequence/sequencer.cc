
#include "config.h"
#include "function.h"

#include "seq_common.h"
#include "sequencer.h"

namespace vt { namespace seq {

void contextualExecution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
) {
  theSeq->executeInContext(
    seq, is_sequenced, std::forward<SeqCallableType>(callable)
  );
}

}} //end namespace vt::seq
