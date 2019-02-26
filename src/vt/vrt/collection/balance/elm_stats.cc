/*
//@HEADER
// ************************************************************************
//
//                          elm_stats.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

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
    lb, node,
    "ElementStats: startTime: time={}\n",
    start_time
  );
}

void ElementStats::stopTime() {
  auto const& stop_time = timing::Timing::getCurrentTime();
  auto const& total_time = stop_time - cur_time_;
  //vtAssert(cur_time_started_, "Must have started time");
  auto const started = cur_time_started_;
  if (started) {
    cur_time_started_ = false;
    addTime(total_time);
  }

  debug_print(
    lb, node,
    "ElementStats: stopTime: time={}, total={}, started={}\n",
    stop_time, total_time, started
  );
}

void ElementStats::setModelWeight(TimeType const& time) {
  cur_time_started_ = false;
  addTime(time);

  debug_print(
    lb, node,
    "ElementStats: setModelWeight: time={}, cur_load={}\n",
    time, phase_timings_.at(cur_phase_)
  );
}

void ElementStats::addTime(TimeType const& time) {
  phase_timings_.resize(cur_phase_ + 1);
  phase_timings_.at(cur_phase_) += time;

  debug_print(
    lb, node,
    "ElementStats: addTime: time={}, cur_load={}\n",
    time, phase_timings_.at(cur_phase_)
  );
}

void ElementStats::updatePhase(PhaseType const& inc) {
  debug_print(
    lb, node,
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
    lb, node,
    "ElementStats: getLoad: load={}, phase={}, size={}\n",
    total_load, phase, phase_timings_.size()
  );

  vtAssert(phase_timings_.size() >= phase, "Must have phase");
  return total_load;
}

}}}} /* end namespace vt::vrt::collection::balance */
