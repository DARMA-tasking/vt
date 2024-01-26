/*
//@HEADER
// *****************************************************************************
//
//                              workload_replay.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/workload_replay.h"
#include "vt/vrt/collection/balance/lb_data_holder.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/phase/phase_manager.h"
#include "vt/utils/json/json_reader.h"

#include <nlohmann/json.hpp>

#include <set>

namespace vt { namespace vrt { namespace collection {
namespace balance { namespace replay {

void replayWorkloads(
  PhaseType initial_phase, PhaseType phases_to_run, PhaseType phase_mod
) {
  // read in object loads from json files
  auto const filename = theConfig()->getLBDataFileIn();
  auto workloads = readInWorkloads(filename);

  // use the default stats handler
  auto stats_cb = vt::theCB()->makeBcast<
    &LBManager::statsHandler
  >(theLBManager()->getProxy());

  replayWorkloads(initial_phase, phases_to_run, phase_mod, workloads, stats_cb);
}

void replayWorkloads(
  PhaseType initial_phase, PhaseType phases_to_run, PhaseType phase_mod,
  std::shared_ptr<LBDataHolder> workloads,
  Callback<std::vector<balance::LoadData>> stats_cb
) {
  using ObjIDType = elm::ElementIDStruct;

  auto const this_rank = theContext()->getNodeStrong();

  // remember vt's base load model
  auto base_load_model = theLBManager()->getBaseLoadModel();
  // force it to use our given workloads, not anything it may have collected
  base_load_model->setLoads(
    &(workloads->node_data_), &(workloads->node_comm_),
    &(workloads->user_defined_lb_info_)
  );
  // point the load model at the workloads for the relevant phase
  runInEpochCollective("WorkloadReplayDriver -> updateLoads", [=] {
    base_load_model->updateLoads(initial_phase);
  });

  // allow remembering what objects are here after the load balancer migrates
  std::set<ObjIDType> migratable_objects_here;
  for (auto workload_id : *base_load_model) {
    if (workload_id.isMigratable()) {
      migratable_objects_here.insert(workload_id);
    }
  }

  // simulate the given number of phases
  auto stop_phase = initial_phase + phases_to_run;
  for (PhaseType phase = initial_phase; phase < stop_phase; phase++) {
    PhaseType input_phase = phase_mod == 0 ? phase : phase % phase_mod;

    // reapply the base load model if in case we overwrote it on a previous iter
    theLBManager()->setLoadModel(base_load_model);

    // force it to use our given workloads, not anything it may have collected
    base_load_model->setLoads(
      &(workloads->node_data_), &(workloads->node_comm_),
      &(workloads->user_defined_lb_info_)
    );

    // point the load model at the workloads for the relevant phase
    runInEpochCollective("WorkloadReplayDriver -> updateLoads", [=] {
      base_load_model->updateLoads(input_phase);
    });

    if (theConfig()->vt_debug_replay) {
      size_t count = 0;
      for (auto workload_id : *base_load_model) {
        if (workload_id.isMigratable()) {
          ++count;
          vt_debug_print(
            normal, replay,
            "workload for element {} is here on input_phase {}\n", workload_id, input_phase
          );
        }
      }
      vt_debug_print(
        terse, replay,
        "Number of known workloads: {}\n", count
      );
    }

    auto pre_lb_load_model = base_load_model;

    // if this isn't the initial phase, then the workload may exist on a rank
    // other than where the objects are currently meant to exist; we will
    // use a Reassignment object to get those workloads where they need to be
    if (phase > initial_phase) {
      if (this_rank == 0) {
        vt_print(
          replay, "Migrating object workloads to phase {} ranks...\n", phase
        );
      }

      // get the workloads to the ranks where the objects currently exist
      pre_lb_load_model = WorkloadDataMigrator::relocateWorkloadsForReplay(
        base_load_model, migratable_objects_here
      );

      // update the load model that will be used by the real load balancer
      theLBManager()->setLoadModel(pre_lb_load_model);

      // force it to use our given workloads, not anything it may have collected
      pre_lb_load_model->setLoads(
        &(workloads->node_data_), &(workloads->node_comm_),
        &(workloads->user_defined_lb_info_)
      );
    }

    if (this_rank == 0) {
      vt_print(replay, "Simulating phase {} using inputs from phase {}...\n", phase, input_phase);
    }

    if (theConfig()->vt_debug_replay) {
      size_t count = 0;
      for (auto workload_id : *pre_lb_load_model) {
        if (workload_id.isMigratable()) {
          ++count;
          vt_debug_print(
            normal, replay,
            "element {} is here on phase {} before LB\n", workload_id, phase
          );
        }
      }
      vt_debug_print(
        terse, replay,
        "Number of objects before LB: {}\n", count
      );
    }

    vt_debug_print(
      terse, replay,
      "constructing load model from real load balancer\n"
    );

    runInEpochCollective("WorkloadReplayDriver -> runRealLB", [&] {
      // run the load balancer but don't let it automatically migrate;
      // instead, remember where the LB wanted to migrate objects

      std::shared_ptr<ProposedReassignment> proposed_model = nullptr;
      auto postLBWork = [&](ReassignmentMsg *msg) {
        auto lb_reassignment = msg->reassignment;
        if (lb_reassignment) {
          proposed_model = std::make_shared<ProposedReassignment>(
            pre_lb_load_model,
            WorkloadDataMigrator::updateCurrentNodes(lb_reassignment)
          );
          migratable_objects_here.clear();
          for (auto it = proposed_model->begin(); it.isValid(); ++it) {
            if ((*it).isMigratable()) {
              migratable_objects_here.insert(*it);
              vt_debug_print(
                 normal, replay,
                "element {} is here on phase {} after LB\n", *it, phase
              );
            }
          }
          auto last_phase_info = theLBManager()->getPhaseInfo();
          last_phase_info->migration_count = lb_reassignment->global_migration_count;
          last_phase_info->ran_lb = true;
          last_phase_info->phase = phase;
        }
        vt_debug_print(
          terse, replay,
          "Number of objects after LB: {}\n", migratable_objects_here.size()
        );
        runInEpochCollective("postLBWorkForReplay -> computeStats", [=] {
          theLBManager()->setComputingBeforeLBStats(false);
          theLBManager()->computeStatistics(
            proposed_model, false, phase, stats_cb
          );
        });
      };
      auto cb = theCB()->makeFunc<ReassignmentMsg>(
        vt::pipe::LifetimeEnum::Once, postLBWork
      );
      auto lb = theLBManager()->decideLBToRun(phase, true);
      auto const start_time = timing::getCurrentTime();
      theLBManager()->startLB(input_phase, lb, cb);
      auto const total_time = timing::getCurrentTime() - start_time;
      if (lb != LBType::NoLB) {
        vt_print(replay, "Time in load balancer: {}\n", total_time);
      }
    });
    runInEpochCollective("WorkloadReplayDriver -> destroyLB", [&] {
      theLBManager()->destroyLB();
    });
    auto last_phase_info = theLBManager()->getPhaseInfo();
    last_phase_info->phase = phase;
    thePhase()->printSummary(last_phase_info);
  }
}

std::shared_ptr<LBDataHolder>
readInWorkloads(const std::string &filename) {
  using util::json::Reader;

  Reader r{filename};
  auto json = r.readFile();
  auto sd = std::make_shared<LBDataHolder>(*json);

  for (auto &phase_data : sd->node_data_) {
    vt_debug_print(
      normal, replay,
      "found {} loads for phase {}\n",
      phase_data.second.size(), phase_data.first
    );
  }

  for (auto &phase_data : sd->node_comm_) {
    vt_debug_print(
      normal, replay,
      "found {} comms for phase {}\n",
      phase_data.second.size(), phase_data.first
    );
  }

  return sd;
}


/*static*/
objgroup::proxy::Proxy<WorkloadDataMigrator>
WorkloadDataMigrator::construct(std::shared_ptr<LoadModel> model_base) {
  auto my_proxy = theObjGroup()->makeCollective<WorkloadDataMigrator>(
    "WorkloadDataMigrator"
  );
  auto strat = my_proxy.get();
  auto base_proxy = my_proxy.template castToBase<lb::BaseLB>();
  vt_debug_print(
    verbose, replay,
    "WorkloadDataMigrator proxy={} base_proxy={}\n",
    my_proxy.getProxy(), base_proxy.getProxy()
  );
  strat->proxy_ = base_proxy;
  strat->load_model_ = model_base.get();
  return my_proxy;
}

void WorkloadDataMigrator::runLB(LoadType) { }

void WorkloadDataMigrator::inputParams(ConfigEntry* spec) { }

std::unordered_map<std::string, std::string>
WorkloadDataMigrator::getInputKeysWithHelp() {
  std::unordered_map<std::string, std::string> const keys_help;
  return keys_help;
}

/*static*/
std::shared_ptr<Reassignment>
WorkloadDataMigrator::updateCurrentNodes(
  std::shared_ptr<const Reassignment> lb_reassignment
) {
  auto modified_reassignment = std::make_shared<Reassignment>();
  modified_reassignment->node_ = lb_reassignment->node_;
  modified_reassignment->global_migration_count =
    lb_reassignment->global_migration_count;
  for (auto &dep : lb_reassignment->depart_) {
    ObjIDType id = dep.first;
    NodeT dest = dep.second;
    id.curr_node = dest;
    modified_reassignment->depart_[id] = dest;
  }
  auto const this_rank = vt::theContext()->getNodeStrong();
  for (auto &arr : lb_reassignment->arrive_) {
    ObjIDType id = arr.first;
    id.curr_node = this_rank;
    modified_reassignment->arrive_[id] = arr.second;
  }
  return modified_reassignment;
}

/*static*/
std::shared_ptr<ProposedReassignment>
WorkloadDataMigrator::relocateWorkloadsForReplay(
  std::shared_ptr<LoadModel> model_base,
  std::set<ObjIDType> migratable_objects_here
) {
  // Object workloads may exist on arbitrary ranks instead of being colocated
  // with the objects themselves. Relocate the workloads to where the objects
  // themselves exist. Do this by first migrating home all workloads that are
  // neither at home nor colocated with the object. Finally, migrate from home
  // all workloads not already colocated with the object.

  std::shared_ptr<ProposedReassignment> move_home_model =
    relocateMisplacedWorkloadsHome(model_base, migratable_objects_here);

  std::shared_ptr<ProposedReassignment> move_here_model =
    relocateMisplacedWorkloadsHere(move_home_model, migratable_objects_here);

  return move_here_model;
}

/*static*/
std::shared_ptr<ProposedReassignment>
WorkloadDataMigrator::relocateMisplacedWorkloadsHome(
  std::shared_ptr<LoadModel> model_base,
  std::set<ObjIDType> migratable_objects_here
) {
  std::shared_ptr<ProposedReassignment> move_home_model = nullptr;

  runInEpochCollective("WorkloadDataMigrator -> migrateLBDataHome", [&] {
    auto norm_lb_proxy = WorkloadDataMigrator::construct(model_base);
    auto normalizer = norm_lb_proxy.get();
    move_home_model = normalizer->createModelToMoveWorkloadsHome(
      model_base, migratable_objects_here
    );
    norm_lb_proxy.destroyCollective();
  });

  return move_home_model;
}

/*static*/
std::shared_ptr<ProposedReassignment>
WorkloadDataMigrator::relocateMisplacedWorkloadsHere(
  std::shared_ptr<LoadModel> model_base,
  std::set<ObjIDType> migratable_objects_here
) {
  std::shared_ptr<ProposedReassignment> move_here_model = nullptr;

  runInEpochCollective("WorkloadDataMigrator -> migrateLBDataHere", [&] {
    auto norm_lb_proxy = WorkloadDataMigrator::construct(model_base);
    auto normalizer = norm_lb_proxy.get();
    move_here_model = normalizer->createModelToMoveWorkloadsHere(
      model_base, migratable_objects_here
    );
    norm_lb_proxy.destroyCollective();
  });

  return move_here_model;
}

std::shared_ptr<ProposedReassignment>
WorkloadDataMigrator::createModelToMoveWorkloadsHome(
  std::shared_ptr<LoadModel> model_base,
  std::set<ObjIDType> migratable_objects_here
) {
  auto const this_rank = vt::theContext()->getNodeStrong();
  vt_debug_print(
    terse, replay,
    "constructing load model to get loads from file location to home\n"
  );

  runInEpochCollective("WorkloadDataMigrator -> transferLBDataHome", [&] {
    for (auto workload_id : *model_base) {
      if (workload_id.isMigratable()) {
        // if the object belongs here, do nothing; otherwise, "transfer" it to
        // the home rank so that it can later be sent to the rank holding the
        // object
        if (workload_id.getHomeNode() != this_rank) {
          if (migratable_objects_here.count(workload_id) == 0) {
            vt_debug_print(
              verbose, replay,
              "will transfer load of {} home to {}\n",
              workload_id, workload_id.getHomeNode()
            );
            migrateObjectTo(workload_id, workload_id.getHomeNode());
          }
        }
      }
    }
  });

  auto tmp_assignment = normalizeReassignments();
  auto home_assignment = updateCurrentNodes(tmp_assignment);
  return std::make_shared<ProposedReassignment>(model_base, home_assignment);
}

std::shared_ptr<ProposedReassignment>
WorkloadDataMigrator::createModelToMoveWorkloadsHere(
  std::shared_ptr<LoadModel> model_base,
  std::set<ObjIDType> migratable_objects_here
) {
  auto const this_rank = vt::theContext()->getNodeStrong();
  vt_debug_print(
    terse, replay,
    "constructing load model to get loads from home to here\n"
  );

  runInEpochCollective("WorkloadDataMigrator -> transferLBDataHere", [&] {
    for (auto workload_id : migratable_objects_here) {
      // if the object is already here, do nothing; otherwise, "transfer" it
      // from the home rank so that we will have the needed workload data
      bool workloads_here = false;
      for (auto other_id : *model_base) {
        if (workload_id == other_id) {
          workloads_here = true;
          break;
        }
      }
      if (!workloads_here) {
        // check that this isn't something that should already have been here
        assert(workload_id.getHomeNode() != this_rank);

        vt_debug_print(
          verbose, replay,
          "will transfer load of {} from home {}\n",
          workload_id, workload_id.getHomeNode()
        );
        ObjIDType mod_id = workload_id;
        // Override curr_node to force retrieval from the home rank
        mod_id.curr_node = workload_id.getHomeNode();
        migrateObjectTo(mod_id, this_rank);
      }
    }
  });

  auto tmp_assignment = normalizeReassignments();
  // now restore the curr_node values to reflect the placement of the "real"
  // object
  auto here_assignment = updateCurrentNodes(tmp_assignment);

  return std::make_shared<ProposedReassignment>(model_base, here_assignment);
}

}}}}} /* end namespace vt::vrt::collection::balance::replay */
