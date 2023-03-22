/*
//@HEADER
// *****************************************************************************
//
//                              test_offlinelb.cc
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

#include <vt/transport.h>
#include <vt/vrt/collection/balance/lb_data_restart_reader.h>
#include <vt/utils/json/json_appender.h>

#include <fstream>

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace lb {

#if vt_check_enabled(lblite)

using TestOfflineLB = TestParallelHarness;

struct SimCol : vt::Collection<SimCol, vt::Index1D> {
  struct Msg : vt::CollectionMessage<SimCol> {
    PhaseType iter = 0;
    explicit Msg(PhaseType in_iter) : iter(in_iter) { }
  };
  void handler(Msg* m) {
    auto const this_node = theContext()->getNode();
    auto const num_nodes = theContext()->getNumNodes();
    auto const next_node = (this_node + 1) % num_nodes;
    auto const prev_node = this_node - 1 >= 0 ? this_node - 1 : num_nodes - 1;
    vt_debug_print(terse, lb, "handler: idx={}: elm={}\n", getIndex(), getElmID());
    if (m->iter == 0 or m->iter == 3 or m->iter == 6) {
      EXPECT_EQ(getIndex().x() / 2, this_node);
    } else if (m->iter == 1 or m-> iter == 2) {
      EXPECT_EQ(getIndex().x() / 2, prev_node);
    } else if (m->iter == 4 or m-> iter == 5) {
      EXPECT_EQ(getIndex().x() / 2, next_node);
    }
  }
};

TEST_F(TestOfflineLB, test_offlinelb_1) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;
  using ElementIDStruct = vt::vrt::collection::balance::ElementIDStruct;
  using LoadSummary = vt::vrt::collection::balance::LoadSummary;
  using LBDataRestartReader = vt::vrt::collection::balance::LBDataRestartReader;

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  auto const next_node = (this_node + 1) % num_nodes;
  auto const prev_node = this_node - 1 >= 0 ? this_node - 1 : num_nodes - 1;

  std::unordered_map<PhaseType, std::vector<ElementIDStruct>> ids;
  int len = 2;
  PhaseType num_phases = 7;
  for (int i = 0; i < len; i++) {
    auto id = elm::ElmIDBits::createCollectionImpl(true, i+1, this_node, this_node);
    id.curr_node = this_node;
    ids[0].push_back(id);
    id.curr_node = next_node;
    ids[3].push_back(id);
    id.curr_node = prev_node;
    ids[6].push_back(id);
  }

  for (int i = 0; i < len; i++) {
    auto pid = elm::ElmIDBits::createCollectionImpl(true, i+1, prev_node, this_node);
    auto nid = elm::ElmIDBits::createCollectionImpl(true, i+1, next_node, this_node);
    ids[1].push_back(pid);
    ids[2].push_back(pid);
    ids[4].push_back(nid);
    ids[5].push_back(nid);
  }

  LBDataHolder dh;
  for (PhaseType i = 0; i < num_phases; i++) {
    for (auto&& elm : ids[i]) {
      dh.node_data_[i][elm] = LoadSummary{3};
    }
  }

  using JSONAppender = util::json::Appender<std::stringstream>;
  std::stringstream stream{std::ios_base::out | std::ios_base::in};
  nlohmann::json metadata;
  metadata["type"] = "LBDatafile";
  auto w = std::make_unique<JSONAppender>(
    "phases", metadata, std::move(stream), true
  );
  for (PhaseType i = 0; i < num_phases; i++) {
    auto j = dh.toJson(i);
    w->addElm(*j);
  }
  stream = w->finish();

  theConfig()->vt_lb = true;
  theConfig()->vt_lb_name = "OfflineLB";
  auto up = LBDataRestartReader::construct();
  curRT->theLBDataReader = up.get();
  theLBDataReader()->readLBDataFromStream(std::move(stream));

  vt::Index1D range{2*num_nodes};
  auto proxy = vt::makeCollection<SimCol>("simcol")
    .bounds(range)
    .bulkInsert()
    .wait();

  for (PhaseType i = 0; i < num_phases; i++) {
    runInEpochCollective("run handler", [&]{
      proxy.broadcastCollective<typename SimCol::Msg, &SimCol::handler>(i);
    });
    thePhase()->nextPhaseCollective();
  }
}

#endif

}}}} /* end namespace vt::tests::unit::lb */
