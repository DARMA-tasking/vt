/*
//@HEADER
// *****************************************************************************
//
//                           load_stats_replayer.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/load_stats_replayer.h"
#include "vt/objgroup/manager.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/model/per_collection.h"
#include "vt/vrt/collection/balance/model/stats_replay.h"
#include "vt/vrt/collection/balance/model/stats_replay.impl.h"
#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/utils/json/json_reader.h"

#include <nlohmann/json.hpp>

#include <cinttypes>
#include <fstream>
#include <regex>

namespace vt { namespace vrt { namespace collection { namespace balance {

void LoadStatsReplayer::setProxy(
  objgroup::proxy::Proxy<LoadStatsReplayer> in_proxy
) {
  proxy_ = in_proxy;
}

/*static*/ std::unique_ptr<LoadStatsReplayer> LoadStatsReplayer::construct() {
  auto ptr = std::make_unique<LoadStatsReplayer>();
  auto proxy = theObjGroup()->makeCollective<LoadStatsReplayer>(ptr.get());
  proxy.get()->setProxy(proxy);
  return ptr;
}

void LoadStatsReplayer::startup() {
}

void LoadStatsReplayer::createAndConfigureForReplay(
  std::size_t coll_elms_per_node, std::size_t initial_phase,
  std::size_t phases_to_run
) {
  loads_by_elm_by_phase_ = loadStatsToReplay(
    initial_phase, phases_to_run
  );
  createCollectionAndModel(coll_elms_per_node, initial_phase);
  configureCollectionForReplay(loads_by_elm_by_phase_, initial_phase);
  loads_by_elm_by_phase_.clear();
}

void LoadStatsReplayer::createCollectionAndModel(
  std::size_t coll_elms_per_node, std::size_t initial_phase
) {
  // create a stats-driven collection to mirror the one from the stats files
  vt_debug_print(
    normal, replay,
    "createCollectionAndModel: creating collection with {} elms per node\n",
    coll_elms_per_node
  );
  auto nranks = vt::theContext()->getNumNodes();
  auto range = vt::Index2D(
    static_cast<int>(nranks), static_cast<int>(coll_elms_per_node)
  );
  coll_proxy_ = vt::theCollection()->constructCollective<
    StatsDriven2DCollection, StatsDriven2DCollection::collectionMap
  >(range);
  auto proxy_bits = coll_proxy_.getProxy();

  // create the load model that will allow the stored load stats to be used
  vt_debug_print(
    normal, replay,
    "createCollectionAndModel: creating load model\n"
  );
  auto base = vt::theLBManager()->getBaseLoadModel();
  auto per_col = std::make_shared<
    vt::vrt::collection::balance::PerCollection
  >(base);
  auto replay_model = std::make_shared<StatsReplay<
    StatsDriven2DCollection, Index2D
  >>(base, coll_proxy_);
  per_col->addModel(proxy_bits, replay_model);
  vt::theLBManager()->setLoadModel(per_col);
}

LoadStatsReplayer::ElmPhaseLoadsMapType LoadStatsReplayer::loadStatsToReplay(
  std::size_t initial_phase, std::size_t phases_to_run
) {
  // absorb relevant phases from existing stats files
  vt_debug_print(
    normal, replay,
    "loadStatsToReplay: reading stats from file\n"
  );
  auto loads_by_elm_by_phase = readStats(initial_phase, phases_to_run);
  return loads_by_elm_by_phase;
}

void LoadStatsReplayer::configureCollectionForReplay(
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  configureElementLocations(loads_by_elm_by_phase, initial_phase);
  configureCollectionWithLoads(loads_by_elm_by_phase, initial_phase);
}

LoadStatsReplayer::IndexVec LoadStatsReplayer::getIndexFromElm(ElmIDType elm_id) {
  auto replayer = vt::theLoadStatsReplayer();
  auto idxiter = replayer->elm_to_index_mapping_.find(elm_id);
  vtAssert(
    idxiter != replayer->elm_to_index_mapping_.end(),
    "Element ID to index mapping must be known"
  );
  auto index = idxiter->second;
  return index;
}

template <>
Index1D LoadStatsReplayer::getTypedIndexFromElm(ElmIDType elm_id) {
  IndexVec vec = getIndexFromElm(elm_id);
  vtAssert(vec.size() == 1, "getTypedIndexFromElm: unexpected number of entries in index vector");
  return Index1D(static_cast<int>(vec[0]));
}

template <>
Index2D LoadStatsReplayer::getTypedIndexFromElm(ElmIDType elm_id) {
  IndexVec vec = getIndexFromElm(elm_id);
  vtAssert(vec.size() == 2, "getTypedIndexFromElm: unexpected number of entries in index vector");
  return Index2D(static_cast<int>(vec[0]), static_cast<int>(vec[1]));
}

template <>
Index3D LoadStatsReplayer::getTypedIndexFromElm(ElmIDType elm_id) {
  IndexVec vec = getIndexFromElm(elm_id);
  vtAssert(vec.size() == 3, "getTypedIndexFromElm: unexpected number of entries in index vector");
  return Index3D(static_cast<int>(vec[0]), static_cast<int>(vec[1]), static_cast<int>(vec[2]));
}

void LoadStatsReplayer::addElmToIndexMapping(
  ElmIDType elm_id, Index1D index
) {
  IndexVec vec;
  vec.push_back(index.x());
  addElmToIndexMapping(elm_id, vec);
}

void LoadStatsReplayer::addElmToIndexMapping(
  ElmIDType elm_id, Index2D index
) {
  IndexVec vec;
  vec.push_back(index.x());
  vec.push_back(index.y());
  addElmToIndexMapping(elm_id, vec);
}

void LoadStatsReplayer::addElmToIndexMapping(
  ElmIDType elm_id, Index3D index
) {
  IndexVec vec;
  vec.push_back(index.x());
  vec.push_back(index.y());
  vec.push_back(index.z());
  addElmToIndexMapping(elm_id, vec);
}

void LoadStatsReplayer::addElmToIndexMapping(
  ElmIDType elm_id, IndexVec index
) {
  auto replayer = vt::theLoadStatsReplayer();
  replayer->elm_to_index_mapping_[elm_id] = index;
}

LoadStatsReplayer::ElmPhaseLoadsMapType LoadStatsReplayer::readStats(
  std::size_t initial_phase, std::size_t phases_to_run
) {
  auto const filename = theConfig()->getLBStatsFileIn();
  vt_debug_print(terse, replay, "input file: {}\n", filename);
  // Read the input files
  try {
    auto loads_map = inputStatsFile(filename, initial_phase, phases_to_run);
    return loads_map;
  } catch (std::exception& e) {
    vtAbort(e.what());
  }
  return ElmPhaseLoadsMapType();
}

LoadStatsReplayer::ElmPhaseLoadsMapType LoadStatsReplayer::inputStatsFile(
  std::string const& filename, std::size_t initial_phase,
  std::size_t phases_to_run
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
        loads_by_elm_by_phase[elm_id.id][phase] = load;
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
    addElmToIndexMapping(elm_id.id, vec);
    auto idx = getTypedIndexFromElm<Index2D>(elm_id.id);
    vt_debug_print(
      normal, replay,
      "reading in mapping from elm={}, home={} to index={}\n",
      elm_id.id, elm_id.home_node, idx
    );
    StatsDriven2DCollection::addMapping(idx, elm_id.home_node);
  }

  return loads_by_elm_by_phase;
}

void LoadStatsReplayer::configureElementLocations(
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  // migrate the collection elements to where they exist at initial_phase
  vt_debug_print(
    normal, replay,
    "configureElementLocations: initial_phase={}\n",
    initial_phase
  );
  vt::runInEpochCollective([this, &loads_by_elm_by_phase, initial_phase]{
    migrateInitialObjectsHere(loads_by_elm_by_phase, initial_phase);
  });
}

void LoadStatsReplayer::configureCollectionWithLoads(
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  // stuff the load stats for each collection element into that element itself
  vt_debug_print(
    normal, replay,
    "configureCollectionWithLoads: num_elms={}, initial_phase={}\n",
    loads_by_elm_by_phase.size(), initial_phase
  );
  vt::runInEpochCollective([this, &loads_by_elm_by_phase, initial_phase]{
    // find vt index of each elm id in our local stats files and send message
    // with loads directly to that index
    stuffStatsIntoCollection(loads_by_elm_by_phase, initial_phase);
  });
}

void LoadStatsReplayer::migrateInitialObjectsHere(
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  // loop over stats elms that were local for initial phase, asking for the
  // corresponding collection elements to be migrated here
  auto const this_rank = vt::theContext()->getNode();
  for (auto item : loads_by_elm_by_phase) {
    auto elm_id = item.first;
    auto &loads_by_phase = item.second;
    auto it = loads_by_phase.find(initial_phase);
    if (it != loads_by_phase.end()) {
      auto index = getTypedIndexFromElm<Index2D>(elm_id);
      if (coll_proxy_[index].tryGetLocalPtr() != nullptr) {
        vt_debug_print(
          normal, replay,
          "index {} (elm {}) is already here\n",
          index, elm_id
        );
      } else {
        vt_debug_print(
          normal, replay,
          "requesting index {} (elm {}) to migrate here\n",
          index, elm_id
        );
        coll_proxy_[index].send<
          StatsDriven2DCollection::MigrateHereMsg,
          &StatsDriven2DCollection::migrateSelf
        >(this_rank);
      }
    }
  }
}

void LoadStatsReplayer::stuffStatsIntoCollection(
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  // sanity check that everybody we're expecting is local by now
  for (auto item : loads_by_elm_by_phase) {
    auto elm_id = item.first;
    auto &loads_by_phase = item.second;
    auto it = loads_by_phase.find(initial_phase);
    if (it != loads_by_phase.end()) {
      auto index = getTypedIndexFromElm<Index2D>(elm_id);
      vtAssert(
        coll_proxy_[index].tryGetLocalPtr() != nullptr,
        "should be local by now"
      );
    }
  }
  // tell the collection what the initial phase is
  auto msg = makeMessage<StatsDriven2DCollection::InitialPhaseMsg>(
    initial_phase
  );
  coll_proxy_.broadcastCollectiveMsg<
    StatsDriven2DCollection::InitialPhaseMsg,
    &StatsDriven2DCollection::setInitialPhase
  >(msg.get());
  // send a message to each elm appearing in our stats files with all
  // relevant loads
  for (auto item : loads_by_elm_by_phase) {
    auto elm_id = item.first;
    auto &loads_by_phase = item.second;
    auto index = getTypedIndexFromElm<Index2D>(elm_id);
    vt_debug_print(
      normal, replay,
      "sending stats for elm {} to index {}\n",
      elm_id, index
    );
    coll_proxy_[index].template send<
      StatsDriven2DCollection::LoadStatsDataMsg,
      &StatsDriven2DCollection::recvLoadStatsData
    >(loads_by_phase);
  }
}

}}}} /* end namespace vt::vrt::collection::balance */
