
#if !defined INCLUDED_LB_INSTRUMENTATION_ENTRY_H
#define INCLUDED_LB_INSTRUMENTATION_ENTRY_H

#include "config.h"
#include "lb/lb_types.h"
#include "timing/timing.h"
#include "timing/timing_type.h"

namespace vt { namespace lb { namespace instrumentation {

struct Entry {
  Entry() = default;

  TimeType begin = 0.0;
  TimeType end = 0.0;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_ENTRY_H*/
