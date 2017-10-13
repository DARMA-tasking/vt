
#if !defined __RUNTIME_TRANSPORT_TERM_COMMON__
#define __RUNTIME_TRANSPORT_TERM_COMMON__

#include "config.h"
#include "messaging/epoch.h"

namespace vt { namespace term {

// Universally covers all messages regardless of associated epoch
static constexpr EpochType const any_epoch_sentinel = -1000;

using TermCounterType = int64_t;
using TermWaveType = int64_t;


}} /* end namespace vt::term */

#endif /*__RUNTIME_TRANSPORT_TERM_COMMON__*/
