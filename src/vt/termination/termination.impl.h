
#if !defined INCLUDED_TERMINATION_TERMINATION_IMPL_H
#define INCLUDED_TERMINATION_TERMINATION_IMPL_H

#include "vt/config.h"
#include "vt/termination/termination.h"
#include "vt/termination/term_common.h"

namespace vt { namespace term {

inline void TerminationDetector::produce(
  EpochType epoch, TermCounterType const& num_units
) {
  debug_print(term, node, "Termination: produce: epoch={}\n",epoch);
  auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
  return produceConsume(in_epoch, num_units, true);
}

inline void TerminationDetector::consume(
  EpochType epoch, TermCounterType const& num_units
) {
  debug_print(term, node, "Termination: consume: epoch={}\n",epoch);
  auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
  return produceConsume(in_epoch, num_units, false);
}

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERMINATION_IMPL_H*/
