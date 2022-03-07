/*
//@HEADER
// *****************************************************************************
//
//                         test_lb_stats_migrator.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "test_collection_common.h"

#include "vt/elm/elm_id.h"
#include "vt/elm/elm_id_bits.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/stats_replay.h"
#include "vt/vrt/collection/balance/model/proposed_reassignment.h"

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit { namespace reassignment {

using namespace vt::tests::unit;

struct TestLBStatsMigrator : TestParallelHarness { };

std::unique_ptr<vt::vrt::collection::balance::StatsData>
setupStats(PhaseType phase, size_t numElements) {
  auto const& this_node = vt::theContext()->getNode();

  using vt::vrt::collection::balance::ElementIDStruct;

  std::vector<ElementIDStruct> myElemList(numElements);

  for (size_t ii = 0; ii < numElements; ++ii) {
    myElemList[ii] = elm::ElmIDBits::createCollectionImpl(
      true, ii+1, this_node, this_node
    );
  }

  using vt::vrt::collection::balance::StatsData;
  auto sd = std::make_unique<StatsData>();

  for (auto&& elmID : myElemList) {
    double tval = elmID.id * 2;
    sd->node_data_[phase][elmID].whole_phase_load = tval;
  }

  return std::move(sd);
}


TEST_F(TestLBStatsMigrator, test_normalize_call) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  using vt::vrt::collection::balance::StatsData;
  auto sd = setupStats(phase, numElements);

  auto base_load_model = vt::theLBManager()->getBaseLoadModel();
  // force it to use our json stats, not anything it may have collected
  base_load_model->setLoads(&sd->node_data_, &sd->node_comm_);

  vt::runInEpochCollective("updateLoads", [&]{
    base_load_model->updateLoads(phase);
  });

  using vt::vrt::collection::balance::LBStatsMigrator;
  vt::objgroup::proxy::Proxy<LBStatsMigrator> norm_lb_proxy;
  using vt::vrt::collection::balance::ProposedReassignment;
  std::shared_ptr<ProposedReassignment> new_model = nullptr;

  // choose a set of migrations for the load model to represent
  vt::runInEpochCollective("do_lb", [&]{
    norm_lb_proxy = LBStatsMigrator::construct(base_load_model);
    auto normalizer = norm_lb_proxy.get();

    vt::runInEpochCollective("choose migrations", [&]{
      for (auto obj_id : *base_load_model) {
        if (obj_id.isMigratable()) {
          vt::NodeType dest = obj_id.id % num_nodes;
          normalizer->migrateObjectTo(obj_id, dest);
        }
      }
    });

    auto reassignment = normalizer->normalizeReassignments();
    new_model = std::make_shared<ProposedReassignment>(
      base_load_model, LBStatsMigrator::updateCurrentNodes(reassignment)
    );
  });
  vt::runInEpochCollective("destroy lb", [&]{
    norm_lb_proxy.destroyCollective();
  });

  // then iterate over it to make sure what shows up here is correct
  for (auto obj_id : *new_model) {
    if (obj_id.isMigratable()) {
      vt::NodeType dest = obj_id.id % num_nodes;
      EXPECT_EQ(dest, this_node);
      EXPECT_EQ(obj_id.getCurrNode(), this_node);

      using vt::vrt::collection::balance::PhaseOffset;
      auto load = new_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
    }
  }
}

TEST_F(TestLBStatsMigrator, test_move_data_home) {
  auto const& this_node = vt::theContext()->getNode();

  PhaseType phase = 0;
  const size_t numElements = 5;

  using vt::vrt::collection::balance::StatsData;
  auto sd = setupStats(phase, numElements);

  auto base_load_model = vt::theLBManager()->getBaseLoadModel();
  // force it to use our json stats, not anything it may have collected
  base_load_model->setLoads(&sd->node_data_, &sd->node_comm_);

  vt::runInEpochCollective("updateLoads", [&]{
    base_load_model->updateLoads(phase);
  });

  using vt::vrt::collection::balance::LBStatsMigrator;
  using vt::vrt::collection::balance::ProposedReassignment;
  using vt::vrt::collection::balance::LBType;
  using ObjIDType = vt::elm::ElementIDStruct;
  std::shared_ptr<ProposedReassignment> not_home_model = nullptr;

  // move everything off the home node
  vt::runInEpochCollective("do shift", [&]{
    auto lb_reassignment = vt::theLBManager()->startLB(phase, LBType::RotateLB);
    if (lb_reassignment != nullptr) {
      fmt::print(
        "{}: global_mig={}, depart={}, arrive={}\n",
        lb_reassignment->node_,
        lb_reassignment->global_migration_count,
        lb_reassignment->depart_.size(),
        lb_reassignment->arrive_.size()
      );
      not_home_model = std::make_shared<ProposedReassignment>(
        base_load_model, LBStatsMigrator::updateCurrentNodes(lb_reassignment)
      );
    }
  });
  runInEpochCollective("destroy lb", [&]{
    vt::theLBManager()->destroyLB();
  });

  // list nothing as here so that we skip the optimization
  std::set<ObjIDType> no_migratable_objects_here;

  vt::objgroup::proxy::Proxy<LBStatsMigrator> norm_lb_proxy;
  std::shared_ptr<ProposedReassignment> back_home_model = nullptr;

  // then create a load model that restores them to homes
  vt::runInEpochCollective("migrate stats home", [&]{
    norm_lb_proxy = LBStatsMigrator::construct(not_home_model);
    auto normalizer = norm_lb_proxy.get();

    back_home_model = normalizer->createStatsAtHomeModel(
      not_home_model, no_migratable_objects_here
    );
  });
  runInEpochCollective("destroy migrator", [&]{
    norm_lb_proxy.destroyCollective();
  });

  // then iterate over it to make sure what shows up here is correct
  for (auto obj_id : *back_home_model) {
    if (obj_id.isMigratable()) {
      auto home = obj_id.getHomeNode();
      EXPECT_EQ(home, this_node);
      EXPECT_EQ(obj_id.getCurrNode(), this_node);

      using vt::vrt::collection::balance::PhaseOffset;
      auto load = back_home_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
    }
  }
}

TEST_F(TestLBStatsMigrator, test_move_some_data_home) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  using vt::vrt::collection::balance::StatsData;
  auto sd = setupStats(phase, numElements);

  auto base_load_model = vt::theLBManager()->getBaseLoadModel();
  // force it to use our json stats, not anything it may have collected
  base_load_model->setLoads(&sd->node_data_, &sd->node_comm_);

  vt::runInEpochCollective("updateLoads", [&]{
    base_load_model->updateLoads(phase);
  });

  using vt::vrt::collection::balance::LBStatsMigrator;
  using vt::vrt::collection::balance::ProposedReassignment;
  using vt::vrt::collection::balance::LBType;
  using ObjIDType = vt::elm::ElementIDStruct;
  std::set<ObjIDType> migratable_objects_here;
  std::shared_ptr<ProposedReassignment> not_home_model = nullptr;

  // move everything off the home node
  vt::runInEpochCollective("do shift", [&]{
    auto lb_reassignment = vt::theLBManager()->startLB(phase, LBType::RotateLB);
    if (lb_reassignment != nullptr) {
      fmt::print(
        "{}: global_mig={}, depart={}, arrive={}\n",
        lb_reassignment->node_,
        lb_reassignment->global_migration_count,
        lb_reassignment->depart_.size(),
        lb_reassignment->arrive_.size()
      );
      not_home_model = std::make_shared<ProposedReassignment>(
        base_load_model, LBStatsMigrator::updateCurrentNodes(lb_reassignment)
      );
      for (auto it = not_home_model->begin(); it.isValid(); ++it) {
        if ((*it).isMigratable()) {
          // only claim a subset of them are here (relates to an optimization in
          // the code being tested)
          if ((*it).id % 3 == 0) {
            migratable_objects_here.insert(*it);
          }
        }
      }
    }
  });
  runInEpochCollective("destroy lb", [&]{
    vt::theLBManager()->destroyLB();
  });

  vt::objgroup::proxy::Proxy<LBStatsMigrator> norm_lb_proxy;
  std::shared_ptr<ProposedReassignment> back_home_if_not_here_model = nullptr;

  // then create a load model that restores them to homes
  vt::runInEpochCollective("migrate stats home", [&]{
    norm_lb_proxy = LBStatsMigrator::construct(not_home_model);
    auto normalizer = norm_lb_proxy.get();

    back_home_if_not_here_model = normalizer->createStatsAtHomeModel(
      not_home_model, migratable_objects_here
    );
  });
  runInEpochCollective("destroy migrator", [&]{
    norm_lb_proxy.destroyCollective();
  });

  // then iterate over it to make sure what shows up here is correct
  for (auto obj_id : *back_home_if_not_here_model) {
    if (obj_id.isMigratable()) {
      auto home = obj_id.getHomeNode();
      if (obj_id.id % 3 == 0) {
        // the optimization should have prevented these from moving home
        EXPECT_EQ(home, (this_node + num_nodes - 1) % num_nodes);
      } else {
        // but these must be home now
        EXPECT_EQ(home, this_node);
      }
      EXPECT_EQ(obj_id.getCurrNode(), this_node);

      using vt::vrt::collection::balance::PhaseOffset;
      auto load = back_home_if_not_here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
    }
  }
}

}}}} // end namespace vt::tests::unit::reassignment

#endif /*vt_check_enabled(lblite)*/
