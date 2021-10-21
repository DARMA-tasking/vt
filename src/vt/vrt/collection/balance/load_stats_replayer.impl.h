/*
//@HEADER
// *****************************************************************************
//
//                         load_stats_replayer.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LOAD_STATS_REPLAYER_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LOAD_STATS_REPLAYER_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/load_stats_replayer.h"
#include "vt/vrt/collection/balance/stats_driven_collection.impl.h"
#include "vt/objgroup/manager.h"
#include "vt/scheduler/scheduler.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/utils/json/json_reader.h"

#include <nlohmann/json.hpp>

#include <cinttypes>
#include <fstream>
#include <regex>

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename IndexType>
void LoadStatsReplayer::emulatePhase(
  CollectionProxy<StatsDrivenCollection<IndexType>> &coll_proxy,
  std::size_t real_phase
) {
  using MsgType = typename StatsDrivenCollection<IndexType>::EmulateMsg;
  vt::runInEpochCollective([=]{
    auto msg = makeMessage<MsgType>(real_phase);
    coll_proxy.template broadcastCollectiveMsg<
      MsgType, &StatsDrivenCollection<IndexType>::emulate
    >(msg.get());
  });
}

template <typename IndexType>
ElmPhaseLoadsMapType LoadStatsReplayer::loadStatsToReplay(
  std::size_t initial_phase, std::size_t phases_to_run,
  StatsDrivenCollectionMapper<IndexType> &mapping
) {
  // absorb relevant phases from existing stats files
  vt_debug_print(
    normal, replay,
    "loadStatsToReplay: reading stats from file\n"
  );
  auto loads_by_elm_by_phase = readStats(initial_phase, phases_to_run, mapping);
  return loads_by_elm_by_phase;
}

template <typename IndexType>
void LoadStatsReplayer::configureCollectionForReplay(
  CollectionProxy<StatsDrivenCollection<IndexType>> &coll_proxy,
  StatsDrivenCollectionMapper<IndexType> &mapping,
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  configureElementLocations(
    coll_proxy, mapping, loads_by_elm_by_phase, initial_phase
  );
  configureCollectionWithLoads(
    coll_proxy, mapping, loads_by_elm_by_phase, initial_phase
  );
}

template <typename IndexType>
ElmPhaseLoadsMapType LoadStatsReplayer::readStats(
  std::size_t initial_phase, std::size_t phases_to_run,
  StatsDrivenCollectionMapper<IndexType> &mapping
) {
  auto const filename = theConfig()->getLBStatsFileIn();
  vt_debug_print(terse, replay, "input file: {}\n", filename);
  // Read the input files
  try {
    auto loads_map = inputStatsFile(
      filename, initial_phase, phases_to_run, mapping
    );
    return loads_map;
  } catch (std::exception& e) {
    vtAbort(e.what());
  }
  return ElmPhaseLoadsMapType();
}

template <typename IndexType>
ElmPhaseLoadsMapType LoadStatsReplayer::inputStatsFile(
  std::string const& filename, std::size_t initial_phase,
  std::size_t phases_to_run, StatsDrivenCollectionMapper<IndexType> &mapping
) {
  using vt::util::json::Reader;
  using vt::vrt::collection::balance::StatsData;

  vt_debug_print(
    normal, replay, "constructing reader\n"
  );
  Reader r{filename};
  vt_debug_print(
    normal, replay, "reading file\n"
  );
  auto json = r.readFile();
  vt_debug_print(
    normal, replay, "constructing stats data\n"
  );
  auto sd = StatsData(*json);

  ElmPhaseLoadsMapType loads_by_elm_by_phase;
  auto stop_phase = initial_phase + phases_to_run;
  for (PhaseType phase = initial_phase; phase < stop_phase; phase++) {
    vt_debug_print(
      terse, replay, "reading phase {} data\n", phase
    );
    try {
      auto &phase_data = sd.node_data_.at(phase);
      for (auto const& entry : phase_data) {
        auto elm_id = entry.first;
        auto load = entry.second;
        vt_debug_print(
          normal, replay,
          "reading in loads for elm={}, home={} on phase={}: load={}\n",
          elm_id.id, elm_id.home_node, phase, load
        );
        loads_by_elm_by_phase[elm_id.id][phase].duration = load;
      }
    } catch (...) {
      auto str = fmt::format("Data for phase {} was not found", phase);
      vtAbort(str);
    }
  }

  vtAssert(sd.node_idx_.size() > 0, "json files must contain vt indices");

  // correctly building the collection map here relies on sd.node_idx_
  // containing phases that show each index on its home rank at least once
  // TODO: make this work without that assumption
  for (auto const &entry : sd.node_idx_) {
    auto &elm_id = entry.first;
    auto &vec = std::get<1>(entry.second);
    mapping.addElmToIndexMapping(elm_id.id, vec);
    auto idx = mapping.getIndexFromElm(elm_id.id);
    vt_debug_print(
      normal, replay,
      "reading in mapping from elm={}, home={} to index={}\n",
      elm_id.id, elm_id.home_node, idx
    );
    mapping.addCollectionMapping(
      idx, elm_id.home_node
    );
  }

  return loads_by_elm_by_phase;
}

template <typename IndexType>
void LoadStatsReplayer::configureElementLocations(
  CollectionProxy<StatsDrivenCollection<IndexType>> &coll_proxy,
  StatsDrivenCollectionMapper<IndexType> &mapping,
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  // migrate the collection elements to where they exist at initial_phase
  vt_debug_print(
    normal, replay,
    "configureElementLocations: initial_phase={}\n",
    initial_phase
  );
  vt::runInEpochCollective([
    &coll_proxy, &loads_by_elm_by_phase, &initial_phase, &mapping
  ]{
    StatsDrivenCollection<IndexType>::migrateInitialObjectsHere(
      coll_proxy, loads_by_elm_by_phase, initial_phase, mapping
    );
  });
}

template <typename IndexType>
void LoadStatsReplayer::configureCollectionWithLoads(
  CollectionProxy<StatsDrivenCollection<IndexType>> &coll_proxy,
  StatsDrivenCollectionMapper<IndexType> &mapping,
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  // stuff the load stats for each collection element into that element itself
  vt_debug_print(
    normal, replay,
    "configureCollectionWithLoads: num_elms={}, initial_phase={}\n",
    loads_by_elm_by_phase.size(), initial_phase
  );
  vt::runInEpochCollective([
    this, &coll_proxy, &loads_by_elm_by_phase, initial_phase, &mapping
  ]{
    // find vt index of each elm id in our local stats files and send message
    // with loads directly to that index
    stuffStatsIntoCollection(
      coll_proxy, mapping, loads_by_elm_by_phase, initial_phase
    );
  });
}

template <typename IndexType>
void LoadStatsReplayer::stuffStatsIntoCollection(
  CollectionProxy<StatsDrivenCollection<IndexType>> &coll_proxy,
  StatsDrivenCollectionMapper<IndexType> &mapping,
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  // sanity check that everybody we're expecting is local by now
  for (auto item : loads_by_elm_by_phase) {
    auto elm_id = item.first;
    auto &loads_by_phase = item.second;
    auto it = loads_by_phase.find(initial_phase);
    if (it != loads_by_phase.end()) {
      auto index = mapping.getIndexFromElm(elm_id);
      vtAssert(
        coll_proxy[index].tryGetLocalPtr() != nullptr,
        "should be local by now"
      );
    }
  }

  // send a message to each elm appearing in our stats files with all
  // relevant loads
  for (auto item : loads_by_elm_by_phase) {
    auto elm_id = item.first;
    auto &loads_by_phase = item.second;
    auto index = mapping.getIndexFromElm(elm_id);
    vt_debug_print(
      normal, replay,
      "sending stats for elm {} to index {}\n",
      elm_id, index
    );
    coll_proxy[index].template send<
      typename StatsDrivenCollection<IndexType>::LoadStatsDataMsg,
      &StatsDrivenCollection<IndexType>::recvLoadStatsData
    >(loads_by_phase);
  }
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LOAD_STATS_REPLAYER_IMPL_H*/
