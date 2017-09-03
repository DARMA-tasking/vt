
#include "common.h"
#include "function.h"

#include "seq_common.h"
#include "sequencer.h"

namespace runtime { namespace seq {

void
contextual_execution(
  seq_t const& seq, bool const& is_sequenced, seq_callable_t&& callable
) {
  the_seq->execute_in_context(
    seq, is_sequenced, std::forward<seq_callable_t>(callable)
  );
}

}} //end namespace runtime::seq
