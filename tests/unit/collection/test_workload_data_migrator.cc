/*
//@HEADER
// *****************************************************************************
//
//                        test_workload_data_migrator.cc
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
#include "vt/vrt/collection/balance/workload_replay.h"
#include "vt/vrt/collection/balance/model/proposed_reassignment.h"

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit { namespace reassignment {

using namespace vt::tests::unit;

using vt::vrt::collection::balance::StatsData;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::ProposedReassignment;
using vt::vrt::collection::balance::WorkloadDataMigrator;

struct TestWorkloadDataMigrator : TestParallelHarness { };

std::shared_ptr<StatsData>
setupWorkloads(PhaseType phase, size_t numElements) {
  auto const& this_node = vt::theContext()->getNode();

  using vt::vrt::collection::balance::ElementIDStruct;

  std::vector<ElementIDStruct> myElemList(numElements);

  for (size_t ii = 0; ii < numElements; ++ii) {
    myElemList[ii] = elm::ElmIDBits::createCollectionImpl(
      true, ii+1, this_node, this_node
    );
  }

  auto sd = std::make_shared<StatsData>();

  for (auto&& elmID : myElemList) {
    double tval = elmID.id * 2;
    sd->node_data_[phase][elmID].whole_phase_load = tval;
    auto &subphase_loads = sd->node_data_[phase][elmID].subphase_loads;
    subphase_loads.push_back(elmID.id % 2 ? tval : 0);
    subphase_loads.push_back(elmID.id % 2 ? 0 : tval);
  }

  return sd;
}

std::shared_ptr<LoadModel>
setupBaseModel(PhaseType phase, std::shared_ptr<StatsData> sd) {
  auto base_load_model = vt::theLBManager()->getBaseLoadModel();
  // force it to use our json workloads, not anything it may have collected
  base_load_model->setLoads(&sd->node_data_, &sd->node_comm_);

  vt::runInEpochCollective("updateLoads", [&]{
    base_load_model->updateLoads(phase);
  });

  return base_load_model;
}

std::shared_ptr<ProposedReassignment>
shiftObjectsRight(
  std::shared_ptr<LoadModel> base_load_model,
  vt::PhaseType phase
) {
  std::shared_ptr<ProposedReassignment> new_model = nullptr;

  vt::runInEpochCollective("do shift", [&]{
    using vt::vrt::collection::balance::LBType;
    auto lb_reassignment = vt::theLBManager()->startLB(phase, LBType::RotateLB);
    if (lb_reassignment != nullptr) {
      vt_debug_print(
        normal, replay,
        "global_mig={}, depart={}, arrive={}\n",
        lb_reassignment->global_migration_count,
        lb_reassignment->depart_.size(),
        lb_reassignment->arrive_.size()
      );
      new_model = std::make_shared<ProposedReassignment>(
        base_load_model,
        WorkloadDataMigrator::updateCurrentNodes(lb_reassignment)
      );
    }
  });

  runInEpochCollective("destroy lb", [&]{
    vt::theLBManager()->destroyLB();
  });

  return new_model;
}

std::shared_ptr<ProposedReassignment>
shiftObjectsRandomly(
  std::shared_ptr<LoadModel> base_load_model,
  vt::PhaseType phase
) {
  std::shared_ptr<ProposedReassignment> new_model = nullptr;

  vt::runInEpochCollective("do shift", [&]{
    using vt::vrt::collection::balance::LBType;
    auto lb_reassignment = vt::theLBManager()->startLB(phase, LBType::RandomLB);
    if (lb_reassignment != nullptr) {
      vt_debug_print(
        normal, replay,
        "global_mig={}, depart={}, arrive={}\n",
        lb_reassignment->global_migration_count,
        lb_reassignment->depart_.size(),
        lb_reassignment->arrive_.size()
      );
      new_model = std::make_shared<ProposedReassignment>(
        base_load_model,
        WorkloadDataMigrator::updateCurrentNodes(lb_reassignment)
      );
    }
  });

  runInEpochCollective("destroy lb", [&]{
    vt::theLBManager()->destroyLB();
  });

  return new_model;
}


TEST_F(TestWorkloadDataMigrator, test_normalize_call) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto sd = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, sd);

  vt::objgroup::proxy::Proxy<WorkloadDataMigrator> norm_lb_proxy;
  std::shared_ptr<ProposedReassignment> new_model = nullptr;

  // choose a set of migrations for the load model to represent
  vt::runInEpochCollective("do_lb", [&]{
    norm_lb_proxy = WorkloadDataMigrator::construct(base_load_model);
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
      base_load_model, WorkloadDataMigrator::updateCurrentNodes(reassignment)
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
      auto subload0 = new_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = new_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_data_home) {
  auto const& this_node = vt::theContext()->getNode();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto sd = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, sd);

  // move everything off the home node
  std::shared_ptr<ProposedReassignment> not_home_model = shiftObjectsRight(
    base_load_model, phase
  );

  // list nothing as here so that we skip the optimization
  using ObjIDType = vt::elm::ElementIDStruct;
  std::set<ObjIDType> no_migratable_objects_here;

  // then create a load model that restores them to homes
  std::shared_ptr<ProposedReassignment> back_home_model =
    WorkloadDataMigrator::relocateMisplacedWorkloadsHome(
      not_home_model, no_migratable_objects_here
    );

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
      auto subload0 = back_home_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = back_home_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_some_data_home) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto sd = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, sd);

  // move everything off the home node
  std::shared_ptr<ProposedReassignment> not_home_model = shiftObjectsRight(
    base_load_model, phase
  );
  using ObjIDType = vt::elm::ElementIDStruct;
  std::set<ObjIDType> migratable_objects_here;
  for (auto it = not_home_model->begin(); it.isValid(); ++it) {
    if ((*it).isMigratable()) {
      // only claim a subset of them are here (relates to an optimization in
      // the code being tested)
      if ((*it).id % 3 == 0) {
        migratable_objects_here.insert(*it);
      }
    }
  }

  // then create a load model that restores them to homes
  std::shared_ptr<ProposedReassignment> back_home_if_not_here_model =
    WorkloadDataMigrator::relocateMisplacedWorkloadsHome(
      not_home_model, migratable_objects_here
    );

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
      auto subload0 = back_home_if_not_here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = back_home_if_not_here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_data_here_from_home) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto sd = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, sd);

  // move everything off the home node
  std::shared_ptr<ProposedReassignment> not_home_model = shiftObjectsRight(
    base_load_model, phase
  );
  using ObjIDType = vt::elm::ElementIDStruct;
  std::set<ObjIDType> migratable_objects_here;
  for (auto it = not_home_model->begin(); it.isValid(); ++it) {
    if ((*it).isMigratable()) {
      migratable_objects_here.insert(*it);
    }
  }

  // then create a load model that restores them to homes
  std::shared_ptr<ProposedReassignment> here_model =
    WorkloadDataMigrator::relocateMisplacedWorkloadsHere(
      base_load_model, migratable_objects_here
    );

  // then iterate over it to make sure what shows up here is correct
  for (auto obj_id : *here_model) {
    if (obj_id.isMigratable()) {
      auto home = obj_id.getHomeNode();
      EXPECT_EQ(home, (this_node + num_nodes - 1) % num_nodes);
      EXPECT_EQ(obj_id.getCurrNode(), this_node);

      using vt::vrt::collection::balance::PhaseOffset;
      auto load = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_some_data_here_from_home) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto sd = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, sd);

  // move everything off the home node
  std::shared_ptr<ProposedReassignment> not_home_model = shiftObjectsRight(
    base_load_model, phase
  );
  using ObjIDType = vt::elm::ElementIDStruct;
  std::set<ObjIDType> migratable_objects_here;
  for (auto it = not_home_model->begin(); it.isValid(); ++it) {
    if ((*it).isMigratable()) {
      // only claim a subset of them are here (relates to an optimization in
      // the code being tested)
      if ((*it).id % 3 == 0) {
        migratable_objects_here.insert(*it);
      }
    }
  }

  // then create a load model that brings them here
  std::shared_ptr<ProposedReassignment> here_model =
    WorkloadDataMigrator::relocateMisplacedWorkloadsHere(
      base_load_model, migratable_objects_here
    );

  // then iterate over it to make sure what shows up here is correct
  for (auto obj_id : *here_model) {
    if (obj_id.isMigratable()) {
      auto home = obj_id.getHomeNode();
      if (obj_id.id % 3 == 0) {
        // these must have moved here from home
        EXPECT_EQ(home, (this_node + num_nodes - 1) % num_nodes);
      } else {
        // but the optimization should have prevented these from moving away
        // from home
        EXPECT_EQ(home, this_node);
      }
      EXPECT_EQ(obj_id.getCurrNode(), this_node);

      using vt::vrt::collection::balance::PhaseOffset;
      auto load = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_data_here_from_whereever_1) {
  auto const& this_node = vt::theContext()->getNode();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto sd = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, sd);

  // shift the workloads to not be home
  std::shared_ptr<ProposedReassignment> workloads_not_home_model =
    shiftObjectsRight(base_load_model, phase);

  // put the objects whereever
  std::shared_ptr<ProposedReassignment> objects_whereever_model =
    shiftObjectsRandomly(base_load_model, phase);
  using ObjIDType = vt::elm::ElementIDStruct;
  std::set<ObjIDType> migratable_objects_here;
  for (auto it = objects_whereever_model->begin(); it.isValid(); ++it) {
    if ((*it).isMigratable()) {
      migratable_objects_here.insert(*it);
    }
  }

  // then create a load model that matches everything up
  std::shared_ptr<ProposedReassignment> here_model =
    WorkloadDataMigrator::relocateWorkloadsForReplay(
      workloads_not_home_model, migratable_objects_here
    );

  // then iterate over it to make sure what shows up here is correct
  for (auto obj_id : *here_model) {
    if (obj_id.isMigratable()) {
      EXPECT_EQ(migratable_objects_here.count(obj_id), 1);

      EXPECT_EQ(obj_id.getCurrNode(), this_node);

      using vt::vrt::collection::balance::PhaseOffset;
      auto load = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_data_here_from_whereever_2) {
  auto const& this_node = vt::theContext()->getNode();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto sd = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, sd);

  // put the workloads whereever
  std::shared_ptr<ProposedReassignment> workloads_whereever_model =
    shiftObjectsRandomly(base_load_model, phase);

  // shift the objects so they aren't at home
  std::shared_ptr<ProposedReassignment> objects_not_home_model =
    shiftObjectsRight(base_load_model, phase);
  using ObjIDType = vt::elm::ElementIDStruct;
  std::set<ObjIDType> migratable_objects_here;
  for (auto it = objects_not_home_model->begin(); it.isValid(); ++it) {
    if ((*it).isMigratable()) {
      migratable_objects_here.insert(*it);
    }
  }

  // then create a load model that matches everything up
  std::shared_ptr<ProposedReassignment> here_model =
    WorkloadDataMigrator::relocateWorkloadsForReplay(
      workloads_whereever_model, migratable_objects_here
    );

  // then iterate over it to make sure what shows up here is correct
  for (auto obj_id : *here_model) {
    if (obj_id.isMigratable()) {
      EXPECT_EQ(migratable_objects_here.count(obj_id), 1);

      EXPECT_EQ(obj_id.getCurrNode(), this_node);

      using vt::vrt::collection::balance::PhaseOffset;
      auto load = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = here_model->getWork(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
    }
  }
}

std::shared_ptr<StatsData>
setupManyWorkloads(
  PhaseType initial_phase, PhaseType num_phases, size_t numElements
) {
  auto const& this_node = vt::theContext()->getNode();

  using vt::vrt::collection::balance::ElementIDStruct;

  std::vector<ElementIDStruct> myElemList(numElements);

  for (size_t ii = 0; ii < numElements; ++ii) {
    myElemList[ii] = elm::ElmIDBits::createCollectionImpl(
      true, ii+1, this_node, this_node
    );
  }

  auto sd = std::make_shared<StatsData>();

  PhaseType stop_phase = initial_phase + num_phases;
  for (PhaseType phase = initial_phase; phase < stop_phase; ++phase) {
    for (size_t ii = 0; ii < numElements; ++ii) {
      auto elmID = myElemList[ii];
      double tval = this_node + (ii + 10) * 2;
      sd->node_data_[phase][elmID].whole_phase_load = tval + phase;
      auto &subphase_loads = sd->node_data_[phase][elmID].subphase_loads;
      subphase_loads.push_back(elmID.id % 2 ? tval : phase);
      subphase_loads.push_back(elmID.id % 2 ? phase : tval);
    }
  }

  auto scrambled_sd = std::make_shared<StatsData>();

  for (PhaseType phase = initial_phase; phase < stop_phase; ++phase) {
    auto base_load_model = setupBaseModel(phase, sd);

    std::shared_ptr<ProposedReassignment> not_home_model =
      shiftObjectsRight(base_load_model, phase);

    std::set<ElementIDStruct> migratable_objects_here;
    for (auto it = not_home_model->begin(); it.isValid(); ++it) {
      if ((*it).isMigratable()) {
        migratable_objects_here.insert(*it);
      }
    }

    // then create a load model that matches everything up
    std::shared_ptr<ProposedReassignment> here_model =
      WorkloadDataMigrator::relocateWorkloadsForReplay(
        not_home_model, migratable_objects_here
      );

    // then store them at their new locations
    for (auto it = here_model->begin(); it.isValid(); ++it) {
      auto obj_id = *it;
      using vt::vrt::collection::balance::PhaseOffset;
      scrambled_sd->node_data_[phase][obj_id].whole_phase_load =
        here_model->getWork(
          obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
        );
      scrambled_sd->node_data_[phase][*it].subphase_loads.push_back(
        here_model->getWork(obj_id, {PhaseOffset::NEXT_PHASE, 0})
      );
      scrambled_sd->node_data_[phase][*it].subphase_loads.push_back(
        here_model->getWork(obj_id, {PhaseOffset::NEXT_PHASE, 1})
      );
    }
  }

  return scrambled_sd;
}

struct TestWorkloadReplay : TestParallelHarness {
#if vt_check_enabled(lblite)
  void addAdditionalArgs() override {
    static char vt_lb[]{"--vt_lb"};
    static char vt_lb_name[]{"--vt_lb_name=RandomLB"};
    addArgs(vt_lb, vt_lb_name);
  }
#endif
};

TEST_F(TestWorkloadReplay, test_run_replay_no_verify) {
  PhaseType initial_phase = 1;
  PhaseType num_phases = 3;
  const size_t numElements = 5;

  // first set up the workloads to replay, moving them around by phase
  auto sd = setupManyWorkloads(initial_phase, num_phases, numElements);

  // then replay them but allow the lb to place objects differently
  replayWorkloads(initial_phase, num_phases, sd);
}

}}}} // end namespace vt::tests::unit::reassignment

#endif /*vt_check_enabled(lblite)*/
