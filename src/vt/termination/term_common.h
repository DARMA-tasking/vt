
#if !defined INCLUDED_TERMINATION_TERM_COMMON_H
#define INCLUDED_TERMINATION_TERM_COMMON_H

#include "config.h"
#include "termination/termination.fwd.h"
#include "epoch/epoch.h"

namespace vt { namespace term {

// Universally covers all messages regardless of associated epoch
static constexpr EpochType const any_epoch_sentinel = -1000;

using TermCounterType = int64_t;
using TermWaveType = int64_t;

}} /* end namespace vt::term */

namespace vt {

extern term::TerminationDetector* theTerm();

} // end namespace vt

#endif /*INCLUDED_TERMINATION_TERM_COMMON_H*/
