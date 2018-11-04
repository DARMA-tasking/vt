
#if !defined INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_H
#define INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_H

#include "vt/config.h"
#include "vt/lb/lb_types.h"
#include "vt/lb/lb_types_internal.h"
#include "vt/lb/instrumentation/database.h"
#include "vt/lb/instrumentation/centralized/collect_msg.h"

namespace vt { namespace lb { namespace instrumentation {

struct CentralCollect {
  static void startReduce(LBPhaseType const& phase);
  static void reduceCurrentPhase();
  static CollectMsg* collectStats(LBPhaseType const& phase);
  static void collectFinished(
    LBPhaseType const& phase, ProcContainerType const& entries
  );
  static LBPhaseType currentPhase();
  static void nextPhase();

  // Active message handlers
  static void centralizedCollect(CollectMsg* msg);
  static void combine(CollectMsg* msg1, CollectMsg* msg2);

private:
  static NodeType collect_root_;
  static LBPhaseType cur_lb_phase_;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_H*/
