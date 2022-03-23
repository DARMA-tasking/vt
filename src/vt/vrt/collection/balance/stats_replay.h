/*
//@HEADER
// *****************************************************************************
//
//                               stats_replay.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_REPLAY_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_REPLAY_H

#include "vt/config.h"
#include "vt/elm/elm_id.h"
#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/vrt/collection/balance/model/proposed_reassignment.h"

#include <string>
#include <unordered_map>
#include <set>

namespace vt { namespace vrt { namespace collection {
namespace balance {

void replayFromInputStats(
  PhaseType initial_phase, PhaseType phases_to_run
);

struct LBStatsMigrator : lb::BaseLB {

  using ObjIDType = elm::ElementIDStruct;

  LBStatsMigrator() = default;

  static objgroup::proxy::Proxy<LBStatsMigrator>
  construct(std::shared_ptr<LoadModel> model_base);

  void runLB(TimeType) override;

  void inputParams(SpecEntry* spec) override;

  static std::unordered_map<std::string, std::string> getInputKeysWithHelp();

  using BaseLB::normalizeReassignments;

  static std::shared_ptr<Reassignment>
  updateCurrentNodes(
    std::shared_ptr<const Reassignment> lb_reassignment
  );

  static std::shared_ptr<StatsData>
  readInWorkloads(std::string filename);

  std::shared_ptr<ProposedReassignment>
  createStatsAtHomeModel(
    std::shared_ptr<LoadModel> model_base,
    std::set<ObjIDType> migratable_objects_here
  );

  std::shared_ptr<ProposedReassignment>
  createStatsHereModel(
    std::shared_ptr<LoadModel> model_base,
    std::set<ObjIDType> migratable_objects_here
  );
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_REPLAY_H*/
