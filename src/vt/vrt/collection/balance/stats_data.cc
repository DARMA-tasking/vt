/*
//@HEADER
// *****************************************************************************
//
//                                stats_data.cc
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

#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/context/context.h"

#include <nlohmann/json.hpp>

namespace vt { namespace vrt { namespace collection { namespace balance {

std::unique_ptr<nlohmann::json> StatsData::toJson(PhaseType phase) const {
  using json = nlohmann::json;

  json j;
  j["id"] = phase;

  std::size_t i = 0;
  for (auto&& elm : node_data_.at(phase)) {
    ElementIDStruct id = elm.first;
    TimeType time = elm.second;
    j["tasks"][i]["resource"] = "cpu";
    j["tasks"][i]["node"] = theContext()->getNode();
    j["tasks"][i]["object"] = id.id;
    j["tasks"][i]["time"] = time;

    auto const& subphase_times = node_subphase_data_.at(phase).at(id);
    std::size_t const subphases = subphase_times.size();
    if (subphases != 0) {
      for (std::size_t s = 0; s < subphases; s++) {
        j["tasks"][i]["subphases"][s]["id"] = s;
        j["tasks"][i]["subphases"][s]["time"] = subphase_times[s];
      }
    }
    i++;
  }

  return std::make_unique<json>(std::move(j));
}

void StatsData::clear() {
  node_comm_.clear();
  node_data_.clear();
  node_subphase_data_.clear();
  node_subphase_comm_.clear();
}

}}}} /* end namespace vt::vrt::collection::balance */
