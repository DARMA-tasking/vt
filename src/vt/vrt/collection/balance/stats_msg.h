
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_STATS_MSG_H
#define INCLUDED_VRT_COLLECTION_BALANCE_STATS_MSG_H

#include "vt/config.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/messaging/message.h"
#include "vt/timing/timing.h"

#include <algorithm>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct LoadData {
  LoadData() = default;
  LoadData(TimeType const& in_load_max, TimeType const& in_load_sum)
    : load_max_(in_load_max), load_sum_(in_load_sum)
  { }

  friend LoadData operator+(LoadData ld1, LoadData const& ld2) {
    auto const& sum_load = ld1.load_sum_ + ld2.load_sum_;
    auto const& max_load = std::max(ld1.load_max_,ld2.load_max_);
    ld1.load_sum_ = sum_load;
    ld1.load_max_ = max_load;
    return ld1;
  }

  TimeType loadMax() const { return load_max_; }
  TimeType loadSum() const { return load_sum_; }

  TimeType load_max_ = 0.0;
  TimeType load_sum_ = 0.0;
};

template <typename ColT>
struct LoadStatsMsg : CollectionMessage<ColT>, LoadData {
  LoadStatsMsg() = default;
  LoadStatsMsg(LoadData const& in_load_data, PhaseType const& phase)
    : LoadData(in_load_data), cur_phase_(phase)
  {}

  PhaseType getPhase() const { return cur_phase_; }

private:
  PhaseType cur_phase_ = fst_lb_phase;
};

struct ProcStatsMsg : collective::ReduceTMsg<LoadData> {
  ProcStatsMsg() = default;
  explicit ProcStatsMsg(TimeType const& in_total_load)
    : ReduceTMsg<LoadData>({in_total_load,in_total_load})
  { }
};

template <typename ColT>
struct StatsMsg : collective::ReduceTMsg<LoadData> {
  using ProxyType = typename ColT::CollectionProxyType;

  StatsMsg() = default;
  StatsMsg(
    PhaseType const& in_cur_phase, TimeType const& in_total_load,
    ProxyType const& in_proxy
  ) : ReduceTMsg<LoadData>({in_total_load,in_total_load}),
      proxy_(in_proxy), cur_phase_(in_cur_phase)
  { }

  ProxyType getProxy() const { return proxy_; }
  PhaseType getPhase() const { return cur_phase_; }
private:
  ProxyType proxy_ = {};
  PhaseType cur_phase_ = fst_lb_phase;
};

struct HierLBMsg : Message {
  HierLBMsg() = default;
  explicit HierLBMsg(PhaseType const& phase)
    : cur_phase_(phase)
  {}

  PhaseType getPhase() const { return cur_phase_; }

private:
  PhaseType cur_phase_ = fst_lb_phase;
};

using GreedyLBMsg = HierLBMsg;
using RotateLBMsg = HierLBMsg;

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_STATS_MSG_H*/
