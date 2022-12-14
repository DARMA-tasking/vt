/*
//@HEADER
// *****************************************************************************
//
//                                 lb_common.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_COMMON_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_COMMON_H

#include "vt/config.h"
#include "vt/elm/elm_id.h"
#include "vt/elm/elm_comm.h"
#include "vt/timing/timing_type.h"
#include "vt/messaging/message/message.h"

#include <nlohmann/json_fwd.hpp>

#include <vector>
#include <unordered_map>
#include <tuple>

namespace vt { namespace vrt { namespace collection {
namespace balance {

using ElementIDStruct = elm::ElementIDStruct;
using ElementIDType = elm::ElementIDType;
using CommMapType = elm::CommMapType;

static constexpr ElementIDType const no_element_id = elm::no_element_id;

/**
 * \brief A description of the interval of interest for a modeled load query
 *
 * The value of `phases` can be in the past or future. Negative values
 * represent a distance into the past, in which -1 is most recent. A
 * value of 0 represents the immediate upcoming phase. Positive values
 * represent more distant future phases.
 */
struct PhaseOffset {
  PhaseOffset() = delete;

  int phases;
  static constexpr unsigned int NEXT_PHASE = 0;

  unsigned int subphase;
  static constexpr unsigned int WHOLE_PHASE = ~0u;
};

struct LoadSummary {
  TimeType whole_phase_load = 0.0;
  std::vector<TimeType> subphase_loads = {};

  TimeType get(PhaseOffset when) const
  {
    if (when.subphase == PhaseOffset::WHOLE_PHASE)
      return whole_phase_load;
    else
      return subphase_loads.at(when.subphase);
  }

  void operator += (const LoadSummary& rhs) {
    vtAssert(subphase_loads.size() == rhs.subphase_loads.size(),
             "Subphase counts must match");

    whole_phase_load += rhs.whole_phase_load;
    for (size_t i = 0; i < subphase_loads.size(); ++i)
      subphase_loads[i] += rhs.subphase_loads[i];
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | whole_phase_load
      | subphase_loads;
  }
};

using LoadMapType         = std::unordered_map<ElementIDStruct, LoadSummary>;
using SubphaseLoadMapType = std::unordered_map<ElementIDStruct, std::vector<TimeType>>;

struct Reassignment {
  // Include the subject node so that these structures can be formed
  // and passed through collectives
  NodeType node_;
  // Global sum reduction result to let the system know whether any
  // distributed structures need to be rebuilt
  int32_t global_migration_count;
  std::unordered_map<ElementIDStruct, NodeType> depart_;
  std::unordered_map<
    ElementIDStruct, std::tuple<LoadSummary, LoadSummary>
  > arrive_;
};

struct ReassignmentMsg : vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_required();

  ReassignmentMsg() = default;
  ReassignmentMsg(ReassignmentMsg const&) = default;
  ReassignmentMsg(
    std::shared_ptr<Reassignment const> in_reassignment, PhaseType in_phase
  ) : reassignment(in_reassignment),
      phase(in_phase)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vtAssert(false, "Must never be called");
  }

  std::shared_ptr<Reassignment const> reassignment;
  PhaseType phase;
};

void applyReassignment(const std::shared_ptr<const balance::Reassignment> &reassignment);

struct LoadModel;

LoadSummary getObjectLoads(
  std::shared_ptr<LoadModel> model, ElementIDStruct object, PhaseOffset when
);

LoadSummary getObjectLoads(
  LoadModel* model, ElementIDStruct object, PhaseOffset when
);

LoadSummary getObjectRawLoads(
  std::shared_ptr<LoadModel> model, ElementIDStruct object, PhaseOffset when
);

LoadSummary getObjectRawLoads(
  LoadModel* model, ElementIDStruct object, PhaseOffset when
);

LoadSummary getNodeLoads(std::shared_ptr<LoadModel> model, PhaseOffset when);

} /* end namespace balance */

namespace lb {

enum struct StatisticQuantity : int8_t {
  min, max, avg, std, var, skw, kur, car, imb, npr, sum
};

enum struct Statistic : int8_t {
  Rank_load_modeled, Rank_load_raw, Rank_comm, Rank_strategy_specific_load_modeled,
  Object_load_modeled, Object_load_raw, Object_comm, Object_strategy_specific_load_modeled,
  // W_l_min, W_l_max, W_l_avg, W_l_std, W_l_var, W_l_skewness, W_l_kurtosis,
  // W_c_min, W_c_max, W_c_avg, W_c_std, W_c_var, W_c_skewness, W_c_kurtosis,
  // W_t_min, W_t_max, W_t_avg, W_t_std, W_t_var, W_t_skewness, W_t_kurtosis,
  // ObjectCardinality,
  ObjectRatio,
  // EdgeCardinality,
  EdgeRatio,
  // ExternalEdgesCardinality,
  // InternalEdgesCardinality
};

using StatisticQuantityMap = std::map<StatisticQuantity, double>;
using StatisticMap = std::unordered_map<Statistic, StatisticQuantityMap>;

nlohmann::json jsonifyPhaseStatistics(const StatisticMap &statistics);

std::unordered_map<Statistic, std::string>& get_lb_stat_names();

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_COMMON_H*/
