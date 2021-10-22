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
#include "vt/vrt/collection/balance/load_stats_replayer.impl.h"
#include "vt/vrt/collection/balance/stats_driven_collection.impl.h"
#include "vt/objgroup/manager.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/model/per_collection.h"
#include "vt/vrt/collection/balance/model/stats_replay.h"
#include "vt/vrt/collection/balance/model/stats_replay.impl.h"

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

void LoadStatsReplayer::create1DAndConfigureForReplay(
  std::size_t coll_elms, std::size_t initial_phase,
  std::size_t phases_to_run
) {
  auto base = vt::theLBManager()->getBaseLoadModel();
  auto replay_model = std::make_shared<StatsReplay<Index1D>>(base);
  auto &mapping = replay_model->getMapping();

  auto loads = loadStatsToReplay(initial_phase, phases_to_run, mapping);
  auto coll_proxy = create1DCollection(
    mapping, coll_elms, initial_phase
  );

  replay_model->setCollectionProxy(coll_proxy);
  auto per_col = std::make_shared<
    vt::vrt::collection::balance::PerCollection
  >(base);
  auto proxy_bits = coll_proxy.getProxy();
  per_col->addModel(proxy_bits, replay_model);
  vt::theLBManager()->setLoadModel(per_col);

  configureCollectionForReplay(coll_proxy, mapping, loads, initial_phase);
}

void LoadStatsReplayer::create2DAndConfigureForReplay(
  std::size_t coll_elms_per_node, std::size_t initial_phase,
  std::size_t phases_to_run
) {
  auto base = vt::theLBManager()->getBaseLoadModel();
  auto replay_model = std::make_shared<StatsReplay<Index2D>>(base);
  auto &mapping = replay_model->getMapping();

  auto loads = loadStatsToReplay(initial_phase, phases_to_run, mapping);
  auto coll_proxy = create2DCollection(
    mapping, coll_elms_per_node, initial_phase
  );

  replay_model->setCollectionProxy(coll_proxy);
  auto per_col = std::make_shared<
    vt::vrt::collection::balance::PerCollection
  >(base);
  auto proxy_bits = coll_proxy.getProxy();
  per_col->addModel(proxy_bits, replay_model);
  vt::theLBManager()->setLoadModel(per_col);

  configureCollectionForReplay(coll_proxy, mapping, loads, initial_phase);
}

CollectionProxy<StatsDrivenCollection<Index1D>>
LoadStatsReplayer::create1DCollection(
  StatsDrivenCollectionMapper<Index1D> &mapping,
  std::size_t coll_elms, std::size_t initial_phase
) {
  // create a stats-driven collection to mirror the one from the stats files
  vt_debug_print(
    normal, replay,
    "create1DCollection: creating 1d collection with {} elms\n",
    coll_elms
  );
  auto range = Index1D(static_cast<int>(coll_elms));
  auto map_proxy = theObjGroup()->makeCollective(&mapping);
  mapping.setProxy(map_proxy);
  mapping.notifyOwners(1);

  auto coll_proxy = vt::makeCollection<StatsDrivenCollection<Index1D>>()
    .bounds(range)
    .bulkInsert()
    .mapperObjGroup<StatsDrivenCollectionMapper<Index1D>>(map_proxy)
    .wait();

  runInEpochCollective([=]{
    // tell the collection what the initial phase is
    coll_proxy.broadcastCollective<
      StatsDrivenCollection<Index1D>::InitialPhaseMsg,
      &StatsDrivenCollection<Index1D>::setInitialPhase
    >(initial_phase);
  });

  return coll_proxy;
}

CollectionProxy<StatsDrivenCollection<Index2D>>
LoadStatsReplayer::create2DCollection(
  StatsDrivenCollectionMapper<Index2D> &mapping,
  std::size_t coll_elms_per_node, std::size_t initial_phase
) {
  // create a stats-driven collection to mirror the one from the stats files
  vt_debug_print(
    normal, replay,
    "create2DCollection: creating 2d collection with {} elms per node\n",
    coll_elms_per_node
  );
  auto nranks = vt::theContext()->getNumNodes();
  auto range = Index2D(
    static_cast<int>(nranks), static_cast<int>(coll_elms_per_node)
  );
  auto map_proxy = theObjGroup()->makeCollective(&mapping);
  mapping.setProxy(map_proxy);
  mapping.notifyOwners(2);

  auto coll_proxy = vt::makeCollection<StatsDrivenCollection<Index2D>>()
    .bounds(range)
    .bulkInsert()
    .mapperObjGroup<StatsDrivenCollectionMapper<Index2D>>(map_proxy)
    .wait();

  runInEpochCollective([=]{
    // tell the collection what the initial phase is
    coll_proxy.broadcastCollective<
      StatsDrivenCollection<Index2D>::InitialPhaseMsg,
      &StatsDrivenCollection<Index2D>::setInitialPhase
    >(initial_phase);
  });

  return coll_proxy;
}

}}}} /* end namespace vt::vrt::collection::balance */
