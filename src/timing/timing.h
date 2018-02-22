
#if !defined INCLUDED_TIMING_TIMING_H
#define INCLUDED_TIMING_TIMING_H

#include "config.h"
#include "timing/timing_type.h"

namespace vt { namespace timing {

struct Timing {
  static TimeType getCurrentTime();
};

}} /* end namespace vt::timing */

#endif /*INCLUDED_TIMING_TIMING_H*/
