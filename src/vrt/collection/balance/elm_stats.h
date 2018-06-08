
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_H

#include "config.h"
#include "vrt/collection/balance/elm_stats.fwd.h"
#include "vrt/collection/balance/phase_msg.h"
#include "vrt/collection/balance/stats_msg.h"
#include "timing/timing.h"
#include "vrt/collection/types/migratable.fwd.h"

#include <cstdint>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct ElementStats {
  using PhaseType = uint64_t;

  ElementStats() = default;
  ElementStats(ElementStats const&) = default;
  ElementStats(ElementStats&&) = default;

  void startTime();
  void stopTime();
  void addTime(TimeType const& time);
  void updatePhase(PhaseType const& inc = 1);
  PhaseType getPhase() const;
  TimeType getLoad(PhaseType const& phase) const;

  template <typename Serializer>
  void serialize(Serializer& s);

public:
  template <typename ColT>
  static void syncNextPhase(PhaseMsg<ColT>* msg, ColT* col);

  template <typename ColT>
  static void computeStats(PhaseMsg<ColT>* msg, ColT* col);

  template <typename ColT>
  static void statsIn(LoadStatsMsg<ColT>* msg, ColT* col);

  template <typename ColT>
  friend struct collection::Migratable;

protected:
  bool cur_time_started_ = false;
  TimeType cur_time_ = 0.0;
  PhaseType cur_phase_ = fst_lb_phase;
  std::vector<TimeType> phase_timings_ = {};
};

template <typename ColT>
struct ComputeStats {
  void operator()(PhaseReduceMsg<ColT>* msg);
};

template <typename ColT>
struct CollectedStats {
  void operator()(StatsMsg<ColT>* msg);
};

template <typename ColT>
struct StartHierLB {
  void operator()(PhaseReduceMsg<ColT>* msg);
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_H*/
