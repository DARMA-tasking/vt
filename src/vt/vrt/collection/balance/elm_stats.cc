
#include "vt/config.h"
#include "vt/vrt/collection/balance/elm_stats.h"
#include "vt/timing/timing.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace balance {

void ElementStats::startTime() {
  auto const& start_time = timing::Timing::getCurrentTime();
  cur_time_ = start_time;
  cur_time_started_ = true;

  debug_print(
    vrt_coll, node,
    "ElementStats: startTime: time={}\n",
    start_time
  );
}

void ElementStats::stopTime() {
  auto const& stop_time = timing::Timing::getCurrentTime();
  auto const& total_time = stop_time - cur_time_;
  //assert(cur_time_started_ && "Must have started time");
  cur_time_started_ = false;
  addTime(total_time);

  debug_print(
    vrt_coll, node,
    "ElementStats: stopTime: time={}, total={}\n",
    stop_time, total_time
  );
}

void ElementStats::addTime(TimeType const& time) {
  phase_timings_.resize(cur_phase_ + 1);
  phase_timings_.at(cur_phase_) += time;

  debug_print(
    vrt_coll, node,
    "ElementStats: addTime: time={}, cur_load={}\n",
    time, phase_timings_.at(cur_phase_)
  );
}

void ElementStats::updatePhase(PhaseType const& inc) {
  debug_print(
    vrt_coll, node,
    "ElementStats: updatePhase: cur_phase_={}, inc={}\n",
    cur_phase_, inc
  );

  phase_timings_.resize(cur_phase_ + 1);
  cur_phase_ += inc;
}

PhaseType ElementStats::getPhase() const {
  return cur_phase_;
}

TimeType ElementStats::getLoad(PhaseType const& phase) const {
  auto const& total_load = phase_timings_.at(phase);

  debug_print(
    vrt_coll, node,
    "ElementStats: getLoad: load={}, phase={}, size={}\n",
    total_load, phase, phase_timings_.size()
  );

  assert(phase_timings_.size() >= phase && "Must have phase");
  return total_load;
}

}}}} /* end namespace vt::vrt::collection::balance */
