/*
//@HEADER
// *****************************************************************************
//
//                                elm_lb_data.h
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

#if !defined INCLUDED_VT_ELM_ELM_LB_DATA_H
#define INCLUDED_VT_ELM_ELM_LB_DATA_H

#include "vt/elm/elm_id.h"
#include "vt/elm/elm_comm.h"
#include "vt/timing/timing.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

struct NodeLBData;

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt { namespace elm {

struct ElementLBData {
  ElementLBData() = default;
  ElementLBData(ElementLBData const&) = default;
  ElementLBData(ElementLBData&&) = default;

  void start(TimeType time);
  void stop(TimeType time);
  void addTime(TimeTypeWrapper const& time);

  void sendToEntity(ElementIDStruct to, ElementIDStruct from, double bytes);
  void sendComm(elm::CommKey key, double bytes);

  void recvComm(elm::CommKey key, double bytes);
  void recvObjData(
    ElementIDStruct to_perm,
    ElementIDStruct from_perm, double bytes, bool bcast
  );
  void recvFromNode(
    ElementIDStruct to_perm, NodeType from,
    double bytes, bool bcast
  );
  void recvToNode(
    NodeType to, ElementIDStruct from_perm,
    double bytes, bool bcast
  );
  void updatePhase(PhaseType const& inc = 1);
  void resetPhase();
  PhaseType getPhase() const;
  TimeType getLoad(PhaseType const& phase) const;
  TimeType getLoad(PhaseType phase, SubphaseType subphase) const;

  CommMapType const& getComm(PhaseType const& phase);
  std::vector<CommMapType> const& getSubphaseComm(PhaseType phase);
  std::vector<TimeType> const& getSubphaseTimes(PhaseType phase);
  void setSubPhase(SubphaseType subphase);
  SubphaseType getSubPhase() const;

  // these are just for unit testing
  std::size_t getLoadPhaseCount() const;
  std::size_t getCommPhaseCount() const;
  std::size_t getSubphaseLoadPhaseCount() const;
  std::size_t getSubphaseCommPhaseCount() const;

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | cur_time_started_;
    s | cur_time_;
    s | cur_phase_;
    s | phase_timings_;
    s | phase_comm_;
    s | cur_subphase_;
    s | subphase_timings_;
    s | subphase_comm_;
  }

  static const constexpr SubphaseType no_subphase =
    std::numeric_limits<SubphaseType>::max();

protected:
  /**
   * \internal \brief Release LB data from phases prior to lookback
   */
  void releaseLBDataFromUnneededPhases(PhaseType phase, unsigned int look_back);

  friend struct vrt::collection::balance::NodeLBData;

protected:
  bool cur_time_started_ = false;
  TimeType cur_time_ = 0.0;
  PhaseType cur_phase_ = fst_lb_phase;
  std::unordered_map<PhaseType, TimeType> phase_timings_ = {};
  std::unordered_map<PhaseType, CommMapType> phase_comm_ = {};

  SubphaseType cur_subphase_ = 0;
  std::unordered_map<PhaseType, std::vector<TimeType>> subphase_timings_ = {};
  std::unordered_map<PhaseType, std::vector<CommMapType>> subphase_comm_ = {};
};

}} /* end namespace vt::elm */

#endif /*INCLUDED_VT_ELM_ELM_LB_DATA_H*/
