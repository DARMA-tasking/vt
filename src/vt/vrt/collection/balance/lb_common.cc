/*
//@HEADER
// *****************************************************************************
//
//                                 lb_common.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/vrt/collection/balance/node_lb_data.h"
#include "vt/scheduler/scheduler.h"

#include <nlohmann/json.hpp>

namespace vt { namespace vrt { namespace collection { namespace balance {

LoadSummary getObjectLoads(
  std::shared_ptr<LoadModel> model, ElementIDStruct object, PhaseOffset when
) {
  return getObjectLoads(model.get(), object, when);
}

LoadSummary getObjectLoads(
  LoadModel* model, ElementIDStruct object, PhaseOffset when
) {
  LoadSummary ret;
  ret.whole_phase_load =
    model->getModeledLoad(object, {when.phases, PhaseOffset::WHOLE_PHASE});

  unsigned int subphases = model->getNumSubphases();
  for (unsigned int i = 0; i < subphases; ++i)
    ret.subphase_loads.push_back(
      model->getModeledLoad(object, {when.phases, i})
    );

  return ret;
}

LoadSummary getObjectRawLoads(
  std::shared_ptr<LoadModel> model, ElementIDStruct object, PhaseOffset when
) {
  return getObjectRawLoads(model.get(), object, when);
}

LoadSummary getObjectRawLoads(
  LoadModel* model, ElementIDStruct object, PhaseOffset when
) {
  LoadSummary ret;

  if (model->hasRawLoad()) {
    ret.whole_phase_load = model->getRawLoad(
      object, {when.phases, PhaseOffset::WHOLE_PHASE}
    );

    unsigned int subphases = model->getNumSubphases();
    for (unsigned int i = 0; i < subphases; ++i)
      ret.subphase_loads.push_back(model->getRawLoad(object, {when.phases, i}));
  }

  return ret;
}

LoadSummary getNodeLoads(std::shared_ptr<LoadModel> model, PhaseOffset when)
{
  LoadSummary ret;

  auto subphases = model->getNumSubphases();
  ret.subphase_loads.resize(subphases, 0.0);

  for (auto obj : *model) {
    ret += getObjectLoads(model, obj, when);
  }

  return ret;
}

void applyReassignment(const std::shared_ptr<const balance::Reassignment> &reassignment) {
  runInEpochCollective([&] {
    auto from = theContext()->getNode();

    for (auto&& departing_elm : reassignment->depart_) {
      auto obj_id = departing_elm.first;
      auto to = departing_elm.second;

      vt_debug_print(
                     normal, lb,
                     "migrateObjectTo, obj_id={}, home={}, from={}, to={}\n",
                     obj_id.id, obj_id.getHomeNode(), from, to
                     );

      theNodeLBData()->migrateObjTo(obj_id, to);
    }
  });
}

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt { namespace vrt { namespace collection { namespace lb {

NLOHMANN_JSON_SERIALIZE_ENUM(StatisticQuantity, {
  {StatisticQuantity::min, "min"},
  {StatisticQuantity::max, "max"},
  {StatisticQuantity::avg, "avg"},
  {StatisticQuantity::std, "std"},
  {StatisticQuantity::var, "var"},
  {StatisticQuantity::skw, "skw"},
  {StatisticQuantity::kur, "kur"},
  {StatisticQuantity::car, "car"},
  {StatisticQuantity::imb, "imb"},
  {StatisticQuantity::npr, "npr"},
  {StatisticQuantity::sum, "sum"},
})

nlohmann::json jsonifyPhaseStatistics(const StatisticMap &statistics) {
  nlohmann::json j;

  for (auto &entry : statistics) {
    auto &name = get_lb_stat_names()[entry.first];
    nlohmann::json &this_stat = j[name];
    for (auto &quant : entry.second) {
      const nlohmann::json quant_name = quant.first;
      this_stat[quant_name.get<std::string>()] = quant.second;
    }
  }

  return j;
}

}}}} /* end namespace vt::vrt::collection::lb */
