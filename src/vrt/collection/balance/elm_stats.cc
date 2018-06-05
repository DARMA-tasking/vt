
#include "config.h"
#include "vrt/collection/balance/elm_stats.h"
#include "timing/timing.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

void ElementStats::startTime() {
  auto const& start_time = timing::Timing::getCurrentTime();
  cur_time_ = start_time;
  cur_time_started_ = true;
}

void ElementStats::stopTime() {
  auto const& stop_time = timing::Timing::getCurrentTime();
  auto const& total_time = stop_time - cur_time_;
  cur_time_started_ = false;
  addTime(total_time);
}

void ElementStats::addTime(TimeType const& time) {
  debug_print_force(
    vrt_coll, node,
    "ElementStats: addTime: time={}\n", time
  );

  phase_timings_.resize(cur_phase_ + 1);
  phase_timings_.at(cur_phase_) += time;
}

void ElementStats::updatePhase(PhaseType const& inc) {
  debug_print_force(
    vrt_coll, node,
    "ElementStats: updatePhase: cur_phase_={}, inc={}\n",
    cur_phase_, inc
  );

  cur_phase_ += inc;
}

PhaseType ElementStats::getPhase() const {
  return cur_phase_;
}

}}}} /* end namespace vt::vrt::collection::balance */
