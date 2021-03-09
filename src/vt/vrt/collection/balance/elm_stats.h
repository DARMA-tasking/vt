/*
//@HEADER
// *****************************************************************************
//
//                                 elm_stats.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/lb_comm.h"
#include "vt/vrt/collection/balance/elm_stats.fwd.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/timing/timing.h"
#include "vt/vrt/collection/types/migratable.fwd.h"

#include <cstdint>
#include <vector>
#include <tuple>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct ElementStats {
  ElementStats() = default;
  ElementStats(ElementStats const&) = default;
  ElementStats(ElementStats&&) = default;

  void startTime();
  void stopTime();
  void addTime(TimeType const& time);
  void recvComm(LBCommKey key, double bytes);
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
  void setSubPhase(SubphaseType subphase);
  SubphaseType getSubPhase() const;

  // these are just for unit testing
  std::size_t getLoadPhaseCount() const;
  std::size_t getCommPhaseCount() const;
  std::size_t getSubphaseLoadPhaseCount() const;
  std::size_t getSubphaseCommPhaseCount() const;

  static const constexpr SubphaseType no_subphase = std::numeric_limits<SubphaseType>::max();
  static void setFocusedSubPhase(VirtualProxyType collection, SubphaseType subphase);
  static SubphaseType getFocusedSubPhase(VirtualProxyType collection);

  template <typename Serializer>
  void serialize(Serializer& s);

public:
  template <typename ColT>
  static void syncNextPhase(CollectStatsMsg<ColT>* msg, ColT* col);

  friend struct collection::Migratable;

protected:
  /**
   * \internal \brief Release stats data from phases prior to lookback
   */
  void releaseStatsFromUnneededPhases(PhaseType phase, unsigned int look_back);

protected:
  bool cur_time_started_ = false;
  TimeType cur_time_ = 0.0;
  PhaseType cur_phase_ = fst_lb_phase;
  std::unordered_map<PhaseType, TimeType> phase_timings_ = {};
  std::unordered_map<PhaseType, CommMapType> phase_comm_ = {};

  SubphaseType cur_subphase_ = 0;
  std::unordered_map<PhaseType, std::vector<TimeType>> subphase_timings_ = {};
  std::unordered_map<PhaseType, std::vector<CommMapType>> subphase_comm_ = {};

  static std::unordered_map<VirtualProxyType, SubphaseType> focused_subphase_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_H*/
