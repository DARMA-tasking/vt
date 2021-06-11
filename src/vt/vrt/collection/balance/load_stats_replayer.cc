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
  std::size_t phases_to_run, bool convert_from_release
) {
  createCollectionAndModel(coll_elms_per_node, initial_phase);
  loads_by_elm_by_phase_ = loadStatsToReplay(
    initial_phase, phases_to_run, convert_from_release
  );
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
  auto replay_model = std::make_shared<StatsReplay>(
    base, coll_proxy_
  );
  per_col->addModel(proxy_bits, replay_model);
  vt::theLBManager()->setLoadModel(per_col);
}

LoadStatsReplayer::ElmPhaseLoadsMapType LoadStatsReplayer::loadStatsToReplay(
  std::size_t initial_phase, std::size_t phases_to_run,
  bool convert_from_release
) {
  // absorb relevant phases from existing stats files
  vt_debug_print(
    normal, replay,
    "loadStatsToReplay: reading stats from file\n"
  );
  auto loads_by_elm_by_phase = readStats(
    initial_phase, phases_to_run, convert_from_release
  );
  return loads_by_elm_by_phase;
}

void LoadStatsReplayer::configureCollectionForReplay(
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
) {
  determineElmToIndexMapping(loads_by_elm_by_phase);
  configureElementLocations(loads_by_elm_by_phase, initial_phase);
  configureCollectionWithLoads(loads_by_elm_by_phase, initial_phase);
}

vt::Index2D LoadStatsReplayer::getIndexFromElm(ElmIDType elm_id) {
  auto replayer = vt::theLoadStatsReplayer();
  auto idxiter = replayer->elm_to_index_mapping_.find(elm_id);
  vtAssert(
    idxiter != replayer->elm_to_index_mapping_.end(),
    "Element ID to index mapping must be known"
  );
  auto index = idxiter->second;
  return index;
}

void LoadStatsReplayer::addElmToIndexMapping(
  ElmIDType elm_id, vt::Index2D index
) {
  auto replayer = vt::theLoadStatsReplayer();
  replayer->elm_to_index_mapping_[elm_id] = index;
}

/*static*/ void LoadStatsReplayer::receiveElmToIndexMapping(
  StatsDriven2DCollection::ElmToIndexMappingMsg* msg
) {
  auto elm_id = msg->elm_id_;
  auto index = msg->index_;
  auto replayer = vt::theLoadStatsReplayer();
  replayer->elm_to_index_mapping_[elm_id] = index;
  vt_debug_print(
    normal, replay,
    "receiveElmToIndexMapping: elm {} is index {}\n",
    elm_id, index
  );
}

LoadStatsReplayer::ElmPhaseLoadsMapType LoadStatsReplayer::readStats(
  std::size_t initial_phase, std::size_t phases_to_run,
  bool convert_from_release
) {
  auto const node = theContext()->getNode();
  std::string const& base_file = theConfig()->vt_lb_stats_file_in;
  std::string const& dir = theConfig()->vt_lb_stats_dir_in;
  auto const file = fmt::format("{}.{}.out", base_file, node);
  auto const filename = fmt::format("{}/{}", dir, file);
  vt_debug_print(terse, replay, "input file: {}\n", filename);
  // Read the input files
  auto loads_map = inputStatsFile(
    filename, initial_phase, phases_to_run, convert_from_release
  );
  return loads_map;
}

LoadStatsReplayer::ElmPhaseLoadsMapType LoadStatsReplayer::inputStatsFile(
  std::string const& filename, std::size_t initial_phase,
  std::size_t phases_to_run, bool convert_from_release
) {
  ElmPhaseLoadsMapType loads_by_elm_by_phase;

  std::ifstream f(filename.c_str());
  std::string line;

  while (std::getline(f, line)) {
    vtAssert(
      std::count(line.begin(), line.end(), ' ') == 0,
      "Spaces in stats file not expected"
    );

    bool is_load_line = false;
    auto pos = line.find('[');
    if (pos != std::string::npos) {
      // this line contains subphase loads notation, i.e.,
      // phase,elm_id,phase_load,n_subphases,[subphase0_load,subphase1_load...]
      is_load_line = true;
    } else if (std::count(line.begin(), line.end(), ',') == 2) {
      // this line does not contain subphase loads notation, i.e.,
      // phase,elm_id,phase_load
      is_load_line = true;
    } // else it is comm data, i.e., phase,elm1_id,elm2_id,bytes,category

    if (is_load_line) {
      std::string nocommas = std::regex_replace(line, std::regex(","), " ");
      std::istringstream iss(nocommas);

      std::size_t phase = 0;
      ElementIDType read_elm_id = 0;
      vt::TimeType load = 0.;
      iss >> phase >> read_elm_id >> load;

      // only load in stats that are strictly necessary, ignoring the rest
      if (phase >= initial_phase && phase < initial_phase + phases_to_run) {
        auto elm_id = convert_from_release ?
          convertReleaseStatsID(read_elm_id) : read_elm_id;
        vt_debug_print(
          normal, replay,
          "reading in loads for elm={}, converted_elm={}, phase={}, load={}\n",
          read_elm_id, elm_id, phase, load
        );
        loads_by_elm_by_phase[elm_id][phase] = load;
      } else {
        vt_debug_print(
          verbose, replay,
          "skipping loads for elm={}, phase={}\n",
          read_elm_id, phase
        );
      }
    } else {
      vt_debug_print(
        verbose, replay,
        "skipping line: {}\n",
        line
      );
    }
  }
  f.close();

  return loads_by_elm_by_phase;
}

LoadStatsReplayer::ElmIDType LoadStatsReplayer::convertReleaseStatsID(
  ElmIDType release_perm_id
) {
  auto local_id = release_perm_id >> 32;
  auto node_id = release_perm_id - (local_id << 32);
  auto converted_local_id = (local_id - 1) / 2;
  auto converted_elm_id = converted_local_id << 32 | node_id;
  return converted_elm_id;
}

void LoadStatsReplayer::determineElmToIndexMapping(
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase
) {
  // empirically determine mapping from elm ids to vt indices
  vt_debug_print(
    normal, replay,
    "determineElmToIndexMapping\n"
  );
  vt::runInEpochCollective([=]{
    coll_proxy_.broadcastCollective<
      StatsDriven2DCollection::NullMsg,
      &StatsDriven2DCollection::shareElmToIndexMapping
    >();
  });
  vt::runInEpochCollective([this, &loads_by_elm_by_phase]{
    requestElmIndices(loads_by_elm_by_phase);
  });
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

void LoadStatsReplayer::requestElmIndices(
  const ElmPhaseLoadsMapType &loads_by_elm_by_phase
) {
  // loop over local stats elms, asking rank determined by hashing perm id what
  // the index is
  auto const this_rank = vt::theContext()->getNode();
  for (auto item : loads_by_elm_by_phase) {
    auto elm_id = item.first;
    vt::NodeType dest = findDirectoryNode(elm_id);
    if (dest != this_rank) {
      vt_debug_print(
        normal, replay,
        "looking for index of elm {}\n",
        elm_id
      );
      auto query = vt::makeMessage<ElmToIndexQueryMsg>(elm_id, this_rank);
      vt::theMsg()->sendMsg<ElmToIndexQueryMsg, requestElmToIndexMapping>(
        dest, query
      );
    }
  }
}

/*static*/ void LoadStatsReplayer::requestElmToIndexMapping(
  ElmToIndexQueryMsg *msg
) {
  auto dest = msg->src_;
  auto elm_id = msg->elm_id_;
  auto replayer = vt::theLoadStatsReplayer();
  auto iter = replayer->elm_to_index_mapping_.find(elm_id);
  if (iter == replayer->elm_to_index_mapping_.end()) {
    vt_print(
      replay,
      "requestElmToIndexMapping: {} asked for index of {} but it is unknown\n",
      dest, elm_id
    );
    vtAbort("unknown id");
  }

  auto index = replayer->getIndexFromElm(elm_id);
  auto response = vt::makeMessage<
    StatsDriven2DCollection::ElmToIndexMappingMsg
  >(index, elm_id);
  vt_debug_print(
    normal, replay,
    "requestElmToIndexMapping: responding that elm {} is index {}\n",
    elm_id, index
  );
  vt::theMsg()->sendMsg<
    StatsDriven2DCollection::ElmToIndexMappingMsg, receiveElmToIndexMapping
  >(dest, response);
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
      auto index = getIndexFromElm(elm_id);
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
      auto index = getIndexFromElm(elm_id);
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
    auto index = getIndexFromElm(elm_id);
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
