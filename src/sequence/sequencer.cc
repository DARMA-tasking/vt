
#include "config.h"
#include "activefn/activefn.h"

#include "seq_common.h"
#include "sequencer_headers.h"

namespace vt { namespace seq {

bool contextualExecution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
) {
  theSeq->lookupContextExecute(seq, std::forward<SeqCallableType>(callable));
  return true;
}

bool contextualExecutionVirtual(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
) {
  theVirtualSeq->lookupContextExecute(
    seq, std::forward<SeqCallableType>(callable)
  );
  return true;
}

}} //end namespace vt::seq
