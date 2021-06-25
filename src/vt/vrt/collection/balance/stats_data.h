/*
//@HEADER
// *****************************************************************************
//
//                                 stats_data.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DATA_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DATA_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/lb_comm.h"

#include <unordered_map>
#include <memory>

#include <nlohmann/json_fwd.hpp>

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct StatsData
 *
 * \brief Data structure that holds LB statistics for a set of phases. Can
 * output them as JSON.
 */
struct StatsData {
  StatsData() = default;

  /**
   * \brief Create \c StatsData from input JSON
   *
   * \param[in] j the json that contains the stats
   */
  StatsData(nlohmann::json const& j);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | node_data_;
    s | node_comm_;
    s | node_subphase_data_;
    s | node_subphase_comm_;
    s | node_idx_;
  }

  /**
   * \brief Output a phase's stats to JSON
   *
   * \param[in] phase the phase
   *
   * \return the json data structure
   */
  std::unique_ptr<nlohmann::json> toJson(PhaseType phase) const;

  /**
   * \brief Clear all statistics
   */
  void clear();

public:
  /// Node timings for each local object
  std::unordered_map<PhaseType, LoadMapType> node_data_;
  /// Node communication graph for each local object
  std::unordered_map<PhaseType, CommMapType> node_comm_;
  /// Node subphase timings for each local object
  std::unordered_map<PhaseType, SubphaseLoadMapType> node_subphase_data_;
  /// Node communication graph for each subphase
  std::unordered_map<PhaseType, std::unordered_map<SubphaseType, CommMapType>> node_subphase_comm_;
  /// Node indices for each ID along with the proxy ID
  std::unordered_map<ElementIDStruct, std::tuple<VirtualProxyType, std::vector<uint64_t>>> node_idx_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DATA_H*/
