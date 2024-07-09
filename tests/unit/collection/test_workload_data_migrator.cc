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
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/lb_data_holder.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/workload_replay.h"
#include "vt/vrt/collection/balance/model/proposed_reassignment.h"

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit { namespace replay {

using namespace vt::tests::unit;

using vt::vrt::collection::balance::LBDataHolder;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::ProposedReassignment;
using vt::vrt::collection::balance::ReassignmentMsg;
using vt::vrt::collection::balance::replay::WorkloadDataMigrator;
using vt::vrt::collection::balance::ElmUserDataType;
using vt::vrt::collection::balance::UserDataValueType;

struct TestWorkloadDataMigrator : TestParallelHarness { };

int computeTag(const elm::ElementIDType &id) {
  return id * 3 + 1;
}

std::shared_ptr<LBDataHolder>
setupWorkloads(PhaseType phase, size_t numElements) {
  auto const& this_node = vt::theContext()->getNode();

  if (this_node == 0) {
    vt_print(replay, "Generating workloads to replay...\n");
  }

  using vt::vrt::collection::balance::ElementIDStruct;

  std::vector<ElementIDStruct> myElemList(numElements);

  for (size_t ii = 0; ii < numElements; ++ii) {
    myElemList[ii] = elm::ElmIDBits::createCollectionImpl(
      true, ii+1, this_node, this_node
    );
  }

  auto lbdh = std::make_shared<LBDataHolder>();

  for (auto&& elmID : myElemList) {
    double tval = elmID.id * 2;
    lbdh->node_data_[phase][elmID].whole_phase_load = tval;
    auto &subphase_loads = lbdh->node_data_[phase][elmID].subphase_loads;
    subphase_loads.push_back(elmID.id % 2 ? tval : 0);
    subphase_loads.push_back(elmID.id % 2 ? 0 : tval);
    lbdh->user_defined_lb_info_[phase][elmID] = ElmUserDataType{
      {"tag", computeTag(elmID.id)}
    };
  }

  return lbdh;
}

std::shared_ptr<LoadModel>
setupBaseModel(PhaseType phase, std::shared_ptr<LBDataHolder> lbdh) {
  auto base_load_model = vt::theLBManager()->getBaseLoadModel();
  // force it to use our json workloads, not anything it may have collected
  base_load_model->setLoads(
    &lbdh->node_data_, &lbdh->node_comm_, &lbdh->user_defined_lb_info_
  );

  vt::runInEpochCollective("updateLoads", [&]{
    base_load_model->updateLoads(phase);
  });

  return base_load_model;
}

std::shared_ptr<ProposedReassignment>
migrateObjects(
  std::shared_ptr<LoadModel> base_load_model,
  vt::PhaseType phase,
  vt::vrt::collection::balance::LBType balancer
) {
  std::shared_ptr<ProposedReassignment> new_model = nullptr;

  vt::runInEpochCollective("migrate", [&]{
    auto postLBWork = [&](ReassignmentMsg *msg) {
      auto lb_reassignment = msg->reassignment;
      if (lb_reassignment) {
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
    };
    auto cb = theCB()->makeFunc<ReassignmentMsg>(
      vt::pipe::LifetimeEnum::Once, postLBWork
    );
    theLBManager()->startLB(phase, balancer, cb);
  });

  runInEpochCollective("destroy lb", [&]{
    vt::theLBManager()->destroyLB();
  });

  return new_model;
}

std::shared_ptr<ProposedReassignment>
shiftObjectsRight(
  std::shared_ptr<LoadModel> base_load_model,
  vt::PhaseType phase
) {
  using vt::vrt::collection::balance::LBType;
  return migrateObjects(base_load_model, phase, LBType::RotateLB);
}

std::shared_ptr<ProposedReassignment>
shiftObjectsRandomly(
  std::shared_ptr<LoadModel> base_load_model,
  vt::PhaseType phase
) {
  using vt::vrt::collection::balance::LBType;
  return migrateObjects(base_load_model, phase, LBType::RandomLB);
}


TEST_F(TestWorkloadDataMigrator, test_normalize_call) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto lbdh = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, lbdh);

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
      auto load = new_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = new_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = new_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
      auto user = new_model->getUserData(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(user.size(), static_cast<std::size_t>(1));
      if (user.size() == 1) {
        auto it = user.find(std::string("tag"));
        EXPECT_TRUE(it != user.end());
        if (it != user.end()) {
          EXPECT_EQ(it->second, UserDataValueType(computeTag(obj_id.id)));
        }
      }
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_data_home) {
  auto const& this_node = vt::theContext()->getNode();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto lbdh = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, lbdh);

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
      auto load = back_home_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = back_home_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = back_home_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
      auto user = back_home_model->getUserData(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(user.size(), static_cast<std::size_t>(1));
      if (user.size() == 1) {
        auto it = user.find(std::string("tag"));
        EXPECT_TRUE(it != user.end());
        if (it != user.end()) {
          EXPECT_EQ(it->second, UserDataValueType(computeTag(obj_id.id)));
        }
      }
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_some_data_home) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto lbdh = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, lbdh);

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
      auto load = back_home_if_not_here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = back_home_if_not_here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = back_home_if_not_here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
      auto user = back_home_if_not_here_model->getUserData(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(user.size(), static_cast<std::size_t>(1));
      if (user.size() == 1) {
        auto it = user.find(std::string("tag"));
        EXPECT_TRUE(it != user.end());
        if (it != user.end()) {
          EXPECT_EQ(it->second, UserDataValueType(computeTag(obj_id.id)));
        }
      }
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_data_here_from_home) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto lbdh = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, lbdh);

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
      auto load = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
      auto user = here_model->getUserData(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(user.size(), static_cast<std::size_t>(1));
      if (user.size() == 1) {
        auto it = user.find(std::string("tag"));
        EXPECT_TRUE(it != user.end());
        if (it != user.end()) {
          EXPECT_EQ(it->second, UserDataValueType(computeTag(obj_id.id)));
        }
      }
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_some_data_here_from_home) {
  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto lbdh = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, lbdh);

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
      auto load = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
      auto user = here_model->getUserData(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(user.size(), static_cast<std::size_t>(1));
      if (user.size() == 1) {
        auto it = user.find(std::string("tag"));
        EXPECT_TRUE(it != user.end());
        if (it != user.end()) {
          EXPECT_EQ(it->second, UserDataValueType(computeTag(obj_id.id)));
        }
      }
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_data_here_from_whereever_1) {
  auto const& this_node = vt::theContext()->getNode();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto lbdh = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, lbdh);

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
      EXPECT_EQ(migratable_objects_here.count(obj_id), static_cast<std::size_t>(1));

      EXPECT_EQ(obj_id.getCurrNode(), this_node);

      using vt::vrt::collection::balance::PhaseOffset;
      auto load = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
      auto user = here_model->getUserData(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(user.size(), static_cast<std::size_t>(1));
      if (user.size() == 1) {
        auto it = user.find(std::string("tag"));
        EXPECT_TRUE(it != user.end());
        if (it != user.end()) {
          EXPECT_EQ(it->second, UserDataValueType(computeTag(obj_id.id)));
        }
      }
    }
  }
}

TEST_F(TestWorkloadDataMigrator, test_move_data_here_from_whereever_2) {
  auto const& this_node = vt::theContext()->getNode();

  PhaseType phase = 0;
  const size_t numElements = 5;

  auto lbdh = setupWorkloads(phase, numElements);
  auto base_load_model = setupBaseModel(phase, lbdh);

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
      EXPECT_EQ(migratable_objects_here.count(obj_id), static_cast<std::size_t>(1));

      EXPECT_EQ(obj_id.getCurrNode(), this_node);

      using vt::vrt::collection::balance::PhaseOffset;
      auto load = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(load, obj_id.id * 2);
      auto subload0 = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 0}
      );
      EXPECT_EQ(subload0, obj_id.id % 2 ? obj_id.id * 2 : 0);
      auto subload1 = here_model->getModeledLoad(
        obj_id, {PhaseOffset::NEXT_PHASE, 1}
      );
      EXPECT_EQ(subload1, obj_id.id % 2 ? 0 : obj_id.id * 2);
      auto user = here_model->getUserData(
        obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(user.size(), static_cast<std::size_t>(1));
      if (user.size() == 1) {
        auto it = user.find(std::string("tag"));
        EXPECT_TRUE(it != user.end());
        if (it != user.end()) {
          EXPECT_EQ(it->second, UserDataValueType(computeTag(obj_id.id)));
        }
      }
    }
  }
}


struct StatsResults {
  StatsResults(PhaseType initial_phase, PhaseType lb_interval)
    : save_phase_(initial_phase),
      comp_phase_(initial_phase),
      lb_interval_(lb_interval) { }

  PhaseType save_phase_ = 0;
  PhaseType comp_phase_ = 0;
  PhaseType lb_interval_ = 1;

  std::unordered_map<PhaseType, double> O_min_;
  std::unordered_map<PhaseType, double> O_max_;
  std::unordered_map<PhaseType, double> O_car_;
  std::unordered_map<PhaseType, double> P_sum_;

  using Statistic = vt::vrt::collection::lb::Statistic;
  using LoadData = vt::vrt::collection::balance::LoadData;

  void saveStatsHandler(std::vector<LoadData> const& in_stat_vec) {
    auto const& this_node = vt::theContext()->getNode();

    if (this_node == 0) {
      vt_print(replay, "Saving subset of statistics for phase {}\n", comp_phase_);
    }

    for (auto&& st : in_stat_vec) {
      auto stat = st.stat_;
      if (stat == Statistic::Rank_load_modeled) {
        P_sum_[save_phase_] = st.sum();
      } else if (stat == Statistic::Object_load_modeled) {
        O_min_[save_phase_] = st.min();
        O_max_[save_phase_] = st.max();
        O_car_[save_phase_] = st.N_;
      }
    }

    ++save_phase_;
  }

  void compStatsHandler(std::vector<LoadData> const& in_stat_vec) {
    auto const& this_node = vt::theContext()->getNode();

    if (this_node == 0) {
      vt_print(replay, "Comparing subset of post-LB statistics for phase {}\n", comp_phase_);
    }

    for (auto&& st : in_stat_vec) {
      auto stat = st.stat_;
      if (stat == Statistic::Rank_load_modeled) {
        EXPECT_EQ(P_sum_[comp_phase_], st.sum());
      } else if (stat == Statistic::Object_load_modeled) {
        EXPECT_EQ(O_min_[comp_phase_], st.min());
        EXPECT_EQ(O_max_[comp_phase_], st.max());
        EXPECT_EQ(O_car_[comp_phase_], st.N_);
      }
    }

    comp_phase_ += lb_interval_;
  }
};

std::shared_ptr<ProposedReassignment>
migrateObjectsAndDoStatistics(
  std::shared_ptr<LoadModel> base_load_model,
  vt::PhaseType phase,
  vt::vrt::collection::balance::LBType balancer,
  vt::objgroup::proxy::Proxy<StatsResults> o_proxy
) {
  std::shared_ptr<ProposedReassignment> new_model = nullptr;

  vt::runInEpochCollective("migrate", [&]{
    auto postLBWork = [&](ReassignmentMsg *msg) {
      auto lb_reassignment = msg->reassignment;
      if (lb_reassignment) {
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
        runInEpochCollective("computeAndStoreStats", [=] {
          auto stats_cb = vt::theCB()->makeBcast<
            &StatsResults::saveStatsHandler
          >(o_proxy);
          theLBManager()->computeStatistics(new_model, false, phase, stats_cb);
        });
      }
    };
    auto cb = theCB()->makeFunc<ReassignmentMsg>(
      vt::pipe::LifetimeEnum::Once, postLBWork
    );
    theLBManager()->startLB(phase, balancer, cb);
  });

  runInEpochCollective("destroy lb", [&]{
    vt::theLBManager()->destroyLB();
  });

  return new_model;
}

std::shared_ptr<ProposedReassignment>
shiftObjectsRightAndDoStatistics(
  std::shared_ptr<LoadModel> base_load_model,
  vt::PhaseType phase, vt::objgroup::proxy::Proxy<StatsResults> o_proxy
) {
  using vt::vrt::collection::balance::LBType;
  return migrateObjectsAndDoStatistics(
    base_load_model, phase, LBType::RotateLB, o_proxy
  );
}

std::shared_ptr<LBDataHolder>
setupManyWorkloads(
  PhaseType initial_phase, PhaseType num_phases, size_t numElements,
  vt::objgroup::proxy::Proxy<StatsResults> o_proxy
) {
  auto const& this_node = vt::theContext()->getNode();

  if (this_node == 0) {
    vt_print(replay, "Generating workloads to replay...\n");
  }

  using vt::vrt::collection::balance::ElementIDStruct;

  std::vector<ElementIDStruct> myElemList(numElements);

  for (size_t ii = 0; ii < numElements; ++ii) {
    myElemList[ii] = elm::ElmIDBits::createCollectionImpl(
      true, ii+1, this_node, this_node
    );
  }

  auto lbdh = std::make_shared<LBDataHolder>();

  PhaseType stop_phase = initial_phase + num_phases;
  for (PhaseType phase = initial_phase; phase < stop_phase; ++phase) {
    for (size_t ii = 0; ii < numElements; ++ii) {
      auto elmID = myElemList[ii];
      double tval = this_node + (ii + 10) * 2;
      lbdh->node_data_[phase][elmID].whole_phase_load = tval + phase;
      auto &subphase_loads = lbdh->node_data_[phase][elmID].subphase_loads;
      subphase_loads.push_back(elmID.id % 2 ? tval : phase);
      subphase_loads.push_back(elmID.id % 2 ? phase : tval);
    }
  }

  auto scrambled_lbdh = std::make_shared<LBDataHolder>();

  for (PhaseType phase = initial_phase; phase < stop_phase; ++phase) {
    auto base_load_model = setupBaseModel(phase, lbdh);

    std::shared_ptr<ProposedReassignment> not_home_model =
      shiftObjectsRightAndDoStatistics(base_load_model, phase, o_proxy);

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
      scrambled_lbdh->node_data_[phase][obj_id].whole_phase_load =
        here_model->getModeledLoad(
          obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
        );
      scrambled_lbdh->node_data_[phase][*it].subphase_loads.push_back(
        here_model->getModeledLoad(obj_id, {PhaseOffset::NEXT_PHASE, 0})
      );
      scrambled_lbdh->node_data_[phase][*it].subphase_loads.push_back(
        here_model->getModeledLoad(obj_id, {PhaseOffset::NEXT_PHASE, 1})
      );
    }
  }

  return scrambled_lbdh;
}

struct TestWorkloadReplay : TestParallelHarness {
#if vt_check_enabled(lblite)
  void addAdditionalArgs() override {
    static char vt_lb[]{"--vt_lb"};
    static char vt_lb_name[]{"--vt_lb_name=RandomLB"};
    static char vt_lb_interval[]{"--vt_lb_interval=2"};
    addArgs(vt_lb, vt_lb_name, vt_lb_interval);
  }
#endif
};

TEST_F(TestWorkloadReplay, test_run_replay_verify_some_stats) {
  PhaseType initial_phase = 1;
  PhaseType num_phases = 5;
  const size_t numElements = 5;
  const PhaseType lb_interval = 2; // make sure this matches the harness above

  auto o_proxy = vt::theObjGroup()->makeCollective<StatsResults>(
    "StatsResults", initial_phase, lb_interval
  );

  // first set up the workloads to replay, moving them around by phase
  auto lbdh = setupManyWorkloads(
    initial_phase, num_phases, numElements, o_proxy
  );

  // make our own stats callback so that we can check the results
  auto stats_cb = vt::theCB()->makeBcast<
    &StatsResults::compStatsHandler
  >(o_proxy);

  // then replay them but allow the lb to place objects differently
  vt::vrt::collection::balance::replay::replayWorkloads(
    initial_phase, num_phases, 0, lbdh, stats_cb
  );
}

}}}} // end namespace vt::tests::unit::replay

#endif /*vt_check_enabled(lblite)*/
