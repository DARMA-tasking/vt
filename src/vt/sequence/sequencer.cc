
#include "vt/config.h"
#include "vt/activefn/activefn.h"

#include "vt/sequence/seq_common.h"
#include "vt/sequence/sequencer_headers.h"

namespace vt { namespace seq {

bool contextualExecution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
) {
  theSeq()->lookupContextExecute(seq, std::forward<SeqCallableType>(callable));
  return true;
}

bool contextualExecutionVirtual(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
) {
  theVirtualSeq()->lookupContextExecute(
    seq, std::forward<SeqCallableType>(callable)
  );
  return true;
}

}} //end namespace vt::seq
