
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H
#define INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H

#include "config.h"
#include "vrt/collection/balance/elm_stats.h"
#include "timing/timing.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename Serializer>
void ElementStats::serialize(Serializer& s) {
  s | cur_time_started_;
  s | cur_time_;
  s | cur_phase_;
  s | phase_timings_;
}

template <typename ColT>
/*static*/ void ElementStats::syncNextPhase(PhaseMsg<ColT>* msg, ColT* col) {
  auto& stats = col->stats_;

  debug_print_force(
    vrt_coll, node,
    "ElementStats: syncNextPhase: stats.getPhase()={}, msg->getPhase()={}\n",
    stats.getPhase(), msg->getPhase()
  );

  assert(stats.getPhase() == msg->getPhase() && "Phases must match");
  stats.updatePhase(1);
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H*/
