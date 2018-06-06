
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H
#define INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H

#include "config.h"
#include "vrt/collection/balance/elm_stats.h"
#include "vrt/collection/balance/phase_msg.h"
#include "vrt/collection/balance/stats_msg.h"
#include "vrt/collection/manager.h"
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
    "ElementStats: syncNextPhase ({}) (idx={}): stats.getPhase()={}, "
    "msg->getPhase()={}\n",
    print_ptr(col), col->getIndex().x(), stats.getPhase(), msg->getPhase()
  );

  assert(stats.getPhase() == msg->getPhase() && "Phases must match");
  stats.updatePhase(1);

  auto const& cur_phase = msg->getPhase();
  auto const& proxy = col->getCollectionProxy();
  auto phase_msg = makeSharedMessage<PhaseReduceMsg<ColT>>(
    cur_phase, col->getProxy()
  );

  theCollection()->reduceMsg<
    ColT,
    PhaseReduceMsg<ColT>,
    PhaseReduceMsg<ColT>::template msgHandler<
      PhaseReduceMsg<ColT>,
      collective::PlusOp<collective::NoneType>,
      ComputeStats<ColT>
    >
  >(proxy, phase_msg, no_epoch, cur_phase);
}

template <typename ColT>
void ComputeStats<ColT>::operator()(PhaseReduceMsg<ColT>* msg) {
  auto const& proxy = msg->getProxy();
  auto const& cur_phase = msg->getPhase();
  auto phase_msg = makeSharedMessage<PhaseMsg<ColT>>(cur_phase,proxy);

  debug_print_force(
    vrt_coll, node,
    "ComputeStats: starting broadcast: phase={}\n", cur_phase
  );

  theCollection()->broadcastMsg<
    PhaseMsg<ColT>,
    ElementStats::computeStats<ColT>
  >(proxy, phase_msg, nullptr, false);
}

template <typename ColT>
/*static*/ void ElementStats::computeStats(PhaseMsg<ColT>* msg, ColT* col) {
  using MsgType = StatsMsg<ColT>;
  auto& stats = col->stats_;
  auto const& cur_phase = msg->getPhase();

  debug_print_force(
    vrt_coll, node,
    "ComputeStats ({}) (idx={}) (reduce): phase={}\n",
    print_ptr(col), col->getIndex().x(), cur_phase
  );

  auto const& total_load = stats.getLoad(cur_phase);
  auto const& proxy = col->getCollectionProxy();
  auto stats_msg = makeSharedMessage<MsgType>(cur_phase, total_load, proxy);

  theCollection()->reduceMsg<
    ColT,
    MsgType,
    MsgType::template msgHandler<
      MsgType, collective::PlusOp<LoadData>, CollectedStats<ColT>
    >
  >(proxy, stats_msg, no_epoch, cur_phase);
}

template <typename ColT>
/*static*/ void ElementStats::statsIn(LoadStatsMsg<ColT>* msg, ColT* col) {
  debug_print_force(
    vrt_coll, node,
    "ElementsStats::statsIn\n"
  );
}

template <typename ColT>
void CollectedStats<ColT>::operator()(StatsMsg<ColT>* msg) {
  auto load_msg = makeSharedMessage<LoadStatsMsg<ColT>>(msg->getConstVal());

  debug_print_force(
    vrt_coll, node,
    "CollectedStats (broadcast): phase={}\n",
    msg->getPhase()
  );

  theCollection()->broadcastMsg<
    LoadStatsMsg<ColT>, ElementStats::statsIn<ColT>
  >(msg->getProxy(), load_msg, nullptr, false);
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H*/
