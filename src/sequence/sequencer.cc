
#include "config.h"
#include "registry_function.h"

#include "seq_common.h"
#include "sequencer.h"

namespace vt { namespace seq {

bool contextualExecution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
) {
  theSeq->lookupContextExecute(seq, std::forward<SeqCallableType>(callable));
  return true;
}

}} //end namespace vt::seq
