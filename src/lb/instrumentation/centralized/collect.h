
#if !defined INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_H
#define INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_H

#include "config.h"
#include "lb/lb_types.h"
#include "lb/instrumentation/database.h"
#include "lb/instrumentation/centralized/collect_msg.h"

namespace vt { namespace lb { namespace instrumentation {

struct CentralCollect {
  static void centralizedCollect(CollectMsg* msg);
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_H*/
