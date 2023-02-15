/*
//@HEADER
// *****************************************************************************
//
//                                elm_lb_data.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_ELM_ELM_LB_DATA_CC
#define INCLUDED_VT_ELM_ELM_LB_DATA_CC

#include "vt/elm/elm_lb_data.h"

#include "vt/config.h"

namespace vt { namespace elm {

void ElementLBData::start(TimeType time) {
  TimeTypeWrapper const start_time = time;
  cur_time_ = start_time.seconds();
  cur_time_started_ = true;

  vt_debug_print(
    verbose, lb,
    "ElementLBData: startTime: time={}\n",
    start_time
  );
}

void ElementLBData::stop(TimeType time) {
  TimeTypeWrapper const stop_time = time;
  TimeTypeWrapper const total_time = stop_time.seconds() - cur_time_;
  //vtAssert(cur_time_started_, "Must have started time");
  auto const started = cur_time_started_;
  if (started) {
    cur_time_started_ = false;
    addTime(total_time);
  }

  vt_debug_print(
    verbose, lb,
    "ElementLBData: stopTime: time={}, total={}, started={}\n",
    stop_time, total_time, started
  );
}

void ElementLBData::sendToEntity(
  ElementIDStruct to, ElementIDStruct from, double bytes
) {
  elm::CommKey key(elm::CommKey::SendRecvTag{}, from, to, false);
  sendComm(key, bytes);
}

void ElementLBData::sendComm(elm::CommKey key, double bytes) {
  phase_comm_[cur_phase_][key].sendMsg(bytes);
  subphase_comm_[cur_phase_].resize(cur_subphase_ + 1);
  subphase_comm_[cur_phase_].at(cur_subphase_)[key].sendMsg(bytes);
}

void ElementLBData::recvComm(
  elm::CommKey key, double bytes
) {
  phase_comm_[cur_phase_][key].receiveMsg(bytes);
  subphase_comm_[cur_phase_].resize(cur_subphase_ + 1);
  subphase_comm_[cur_phase_].at(cur_subphase_)[key].receiveMsg(bytes);
}

void ElementLBData::recvObjData(
  ElementIDStruct pto,
  ElementIDStruct pfrom, double bytes, bool bcast
) {
  elm::CommKey key(elm::CommKey::CollectionTag{}, pfrom, pto, bcast);
  recvComm(key, bytes);
}

void ElementLBData::recvFromNode(
  ElementIDStruct pto, NodeType from,
  double bytes, bool bcast
) {
  elm::CommKey key(elm::CommKey::NodeToCollectionTag{}, from, pto, bcast);
  recvComm(key, bytes);
}

void ElementLBData::recvToNode(
  NodeType to, ElementIDStruct pfrom,
  double bytes, bool bcast
) {
  elm::CommKey key(elm::CommKey::CollectionToNodeTag{}, pfrom, to, bcast);
  recvComm(key, bytes);
}

void ElementLBData::addTime(TimeTypeWrapper const& time) {
  phase_timings_[cur_phase_] += time.seconds();

  subphase_timings_[cur_phase_].resize(cur_subphase_ + 1);
  subphase_timings_[cur_phase_].at(cur_subphase_) += time.seconds();

  vt_debug_print(
    verbose,lb,
    "ElementLBData: addTime: time={}, cur_load={}\n",
    time,
    TimeTypeWrapper(phase_timings_[cur_phase_])
  );
}

void ElementLBData::updatePhase(PhaseType const& inc) {
  vt_debug_print(
    verbose, lb,
    "ElementLBData: updatePhase: cur_phase_={}, inc={}\n",
    cur_phase_, inc
  );

  cur_phase_ += inc;

  // Access all table entries for current phase, to ensure presence even
  // if they're left empty
  phase_timings_[cur_phase_];
  subphase_timings_[cur_phase_];
  phase_comm_[cur_phase_];
  subphase_comm_[cur_phase_];
}

void ElementLBData::resetPhase() {
  cur_phase_ = fst_lb_phase;
}

PhaseType ElementLBData::getPhase() const {
  return cur_phase_;
}

TimeType ElementLBData::getLoad(PhaseType const& phase) const {
  auto iter = phase_timings_.find(phase);
  if (iter != phase_timings_.end()) {
    TimeTypeWrapper const total_load = phase_timings_.at(phase);

    vt_debug_print(
      verbose, lb,
      "ElementLBData: getLoad: load={}, phase={}, size={}\n",
      total_load, phase, phase_timings_.size()
    );

    return total_load.seconds();
  } else {
    return 0.0;
  }
}

TimeType
ElementLBData::getLoad(PhaseType phase, SubphaseType subphase) const {
  if (subphase == no_subphase)
    return getLoad(phase);

  auto const& subphase_loads = subphase_timings_.at(phase);

  vtAssert(subphase_loads.size() > subphase, "Must have subphase");
  TimeTypeWrapper const total_load = subphase_loads.at(subphase);

  vt_debug_print(
    verbose, lb,
    "ElementLBData: getLoad: load={}, phase={}, subphase={}\n",
    total_load, phase, subphase
  );

  return total_load.seconds();
}

std::vector<TimeType> const& ElementLBData::getSubphaseTimes(PhaseType phase) {
  return subphase_timings_[phase];
}

CommMapType const&
ElementLBData::getComm(PhaseType const& phase) {
  auto const& phase_comm = phase_comm_[phase];

  vt_debug_print(
    verbose, lb,
    "ElementLBData: getComm: comm size={}, phase={}\n",
    phase_comm.size(), phase
  );

  return phase_comm;
}

std::vector<CommMapType> const& ElementLBData::getSubphaseComm(PhaseType phase) {
  auto const& subphase_comm = subphase_comm_[phase];

  vt_debug_print(
    verbose, lb,
    "ElementLBData: getSubphaseComm: comm size={}, phase={}\n",
    subphase_comm.size(), phase
  );

  return subphase_comm;
}

void ElementLBData::setSubPhase(SubphaseType subphase) {
  vtAssert(subphase < no_subphase, "subphase must be less than sentinel");
  cur_subphase_ = subphase;
}

SubphaseType ElementLBData::getSubPhase() const {
  return cur_subphase_;
}

void ElementLBData::releaseLBDataFromUnneededPhases(PhaseType phase, unsigned int look_back) {
  if (phase >= look_back) {
    phase_timings_.erase(phase - look_back);
    subphase_timings_.erase(phase - look_back);
    phase_comm_.erase(phase - look_back);
    subphase_comm_.erase(phase - look_back);
  }
}

std::size_t ElementLBData::getLoadPhaseCount() const {
  return phase_timings_.size();
}

std::size_t ElementLBData::getCommPhaseCount() const {
  return phase_comm_.size();
}

std::size_t ElementLBData::getSubphaseLoadPhaseCount() const {
  return subphase_timings_.size();
}

std::size_t ElementLBData::getSubphaseCommPhaseCount() const {
  return subphase_comm_.size();
}

}} /* end namespace vt::elm */

#endif /*INCLUDED_VT_ELM_ELM_LB_DATA_CC*/
