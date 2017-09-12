
#include "common.h"
#include "function.h"

#include "seq_common.h"
#include "sequencer.h"

namespace vt { namespace seq {

void contextual_execution(
  SeqType const& seq, bool const& is_sequenced, SeqCallableType&& callable
) {
  the_seq->execute_in_context(
    seq, is_sequenced, std::forward<SeqCallableType>(callable)
  );
}

}} //end namespace vt::seq
