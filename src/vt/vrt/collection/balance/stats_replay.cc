/*
//@HEADER
// *****************************************************************************
//
//                              stats_replay.cc
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
#include "vt/vrt/collection/balance/stats_replay.h"
#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/utils/json/json_reader.h"

#include <nlohmann/json.hpp>

#include <set>

namespace vt { namespace vrt { namespace collection {
namespace balance {

void replayWorkloads(
  PhaseType initial_phase, PhaseType phases_to_run
) {
  using ObjIDType = elm::ElementIDStruct;

  // read in object loads from json files
  auto const filename = theConfig()->getLBStatsFileIn();
  auto sd = WorkloadDataMigrator::readInWorkloads(filename);

  // remember vt's base load model
  auto base_load_model = theLBManager()->getBaseLoadModel();

  // allow remembering the migrations suggested by the load balancer
  std::shared_ptr<const Reassignment> lb_reassignment = nullptr;

  // allow remembering what objects are here after the load balancer migrates
  std::set<ObjIDType> migratable_objects_here;
  // force it to use our json workloads, not anything it may have collected
  base_load_model->setLoads(&(sd->node_data_), &(sd->node_comm_));
  // point the load model at the workloads for the relevant phase
  runInEpochCollective("WorkloadReplayDriver -> updateLoads", [=] {
    base_load_model->updateLoads(initial_phase);
  });
  for (auto stat_obj_id : *base_load_model) {
    if (stat_obj_id.isMigratable()) {
      migratable_objects_here.insert(stat_obj_id);
    }
  }

  // simulate the requested number of phases
  auto const this_rank = theContext()->getNode();
  auto stop_phase = initial_phase + phases_to_run;
  for (PhaseType phase = initial_phase; phase < stop_phase; phase++) {
    // reapply the base load model if in case we overwrote it on a previous iter
    theLBManager()->setLoadModel(base_load_model);

    // force it to use our json workloads, not anything it may have collected
    base_load_model->setLoads(&(sd->node_data_), &(sd->node_comm_));

    // point the load model at the workloads for the relevant phase
    runInEpochCollective("WorkloadReplayDriver -> updateLoads", [=] {
      base_load_model->updateLoads(phase);
    });

    size_t count = 0;
    for (auto stat_obj_id : *base_load_model) {
      if (stat_obj_id.isMigratable()) {
        ++count;
        vt_debug_print(
          normal, replay,
          "workloads for id {} are here on phase {}\n",
          stat_obj_id, phase
        );
      }
    }
    // sanity output
    vt_debug_print(
      terse, replay,
      "Stats num objects: {}\n", count
    );

    auto pre_lb_load_model = base_load_model;

    // if this isn't the initial phase, then the workloads may exist on a rank
    // other than where the objects are currently meant to exist; we will
    // use a Reassignment object to get those workloads where they need to be
    if (phase > initial_phase) {
      if (this_rank == 0) {
        vt_print(
          replay,
          "Migrating imported object workloads to phase {} ranks...\n",
          phase
        );
      }

      // at the beginning of this phase, objects will exist in the locations
      // they were placed by the previous lb invocation; this will be the
      // arriving node for the purposes of this load model; that location
      // is known by both the rank at which the lb placed the object and the
      // rank from which the lb removed the object; the curr_node member of
      // the object ids in the lb_reassignment object refers to the pre-lb
      // location on the previous phase, but the curr_node member for our new
      // load model must point to where the workloads data exists for this phase

      // the workloads data for this phase can exist at arbitrary locations; the
      // only rank to know the location of this data is the one that has it;
      // this will be the departing node for the purposes of this load model;
      // we need to make sure the curr_node member of the object ids in our
      // new load model points to the node on which the workloads data lives

      runInEpochCollective("WorkloadReplayDriver -> migrateStatsDataHome", [&] {
        auto norm_lb_proxy = WorkloadDataMigrator::construct(base_load_model);
        auto normalizer = norm_lb_proxy.get();
        pre_lb_load_model = normalizer->createModelToMoveWorkloadsHome(
          base_load_model, migratable_objects_here
        );
        norm_lb_proxy.destroyCollective();
      });
      theLBManager()->setLoadModel(pre_lb_load_model);
      pre_lb_load_model->setLoads(&(sd->node_data_), &(sd->node_comm_));

      runInEpochCollective("WorkloadReplayDriver -> migrateStatsDataHere", [&] {
        auto norm_lb_proxy = WorkloadDataMigrator::construct(pre_lb_load_model);
        auto normalizer = norm_lb_proxy.get();
        pre_lb_load_model = normalizer->createModelToMoveWorkloadsHere(
          pre_lb_load_model, migratable_objects_here
        );
        norm_lb_proxy.destroyCollective();
      });
      theLBManager()->setLoadModel(pre_lb_load_model);
      pre_lb_load_model->setLoads(&(sd->node_data_), &(sd->node_comm_));
    }

    if (this_rank == 0) {
      vt_print(replay, "Simulating phase {}...\n", phase);
    }

    // sanity output
    count = 0;
    for (auto stat_obj_id : *pre_lb_load_model) {
      if (stat_obj_id.isMigratable()) {
        ++count;
        vt_debug_print(
          normal, replay,
          "element {} is here on phase {} pre-lb\n",
          stat_obj_id, phase
        );
      }
    }
    vt_debug_print(
      terse, replay,
      "Pre-lb num objects: {}\n", count
    );

    vt_debug_print(
      terse, replay,
      "constructing load model from real load balancer\n"
    );

    runInEpochCollective("WorkloadReplayDriver -> runRealLB", [&] {
      // run the load balancer but don't let it automatically migrate;
      // instead, remember where the LB wanted to migrate objects
      lb_reassignment = theLBManager()->selectStartLB(phase);

      if (lb_reassignment) {
        auto proposed_model = std::make_shared<ProposedReassignment>(
          pre_lb_load_model, lb_reassignment
        );
        migratable_objects_here.clear();
        for (auto it = proposed_model->begin(); it.isValid(); ++it) {
          if ((*it).isMigratable()) {
            ObjIDType loc_id = *it;
            loc_id.curr_node = this_rank;
            migratable_objects_here.insert(loc_id);
            vt_debug_print(
               normal, replay,
              "element {} is here on phase {} post-lb\n",
              loc_id, phase
            );
          }
        }
      }
      vt_debug_print(
        terse, replay,
        "Post-lb num objects: {}\n", migratable_objects_here.size()
      );
    });
    runInEpochCollective("WorkloadReplayDriver -> destroyLB", [&] {
      theLBManager()->destroyLB();
    });
    theCollective()->barrier();
  }
}


/*static*/
objgroup::proxy::Proxy<WorkloadDataMigrator>
WorkloadDataMigrator::construct(std::shared_ptr<LoadModel> model_base) {
  auto my_proxy = theObjGroup()->makeCollective<WorkloadDataMigrator>();
  auto strat = my_proxy.get();
  auto base_proxy = my_proxy.template registerBaseCollective<lb::BaseLB>();
  vt_debug_print(
    verbose, replay,
    "WorkloadDataMigrator proxy={} base_proxy={}\n",
    my_proxy.getProxy(), base_proxy.getProxy()
  );
  strat->proxy_ = base_proxy;
  strat->load_model_ = model_base.get();
  return my_proxy;
}

void WorkloadDataMigrator::runLB(TimeType) { }

void WorkloadDataMigrator::inputParams(SpecEntry* spec) { }

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
    NodeType dest = dep.second;
    id.curr_node = dest;
    modified_reassignment->depart_[id] = dest;
  }
  auto const this_rank = vt::theContext()->getNode();
  for (auto &arr : lb_reassignment->arrive_) {
    ObjIDType id = arr.first;
    id.curr_node = this_rank;
    modified_reassignment->arrive_[id] = arr.second;
  }
  return modified_reassignment;
}

/*static*/
std::shared_ptr<StatsData>
WorkloadDataMigrator::readInWorkloads(std::string filename) {
  using util::json::Reader;

  Reader r{filename};
  auto json = r.readFile();
  auto sd = std::make_shared<StatsData>(*json);

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

std::shared_ptr<ProposedReassignment>
WorkloadDataMigrator::createModelToMoveWorkloadsHome(
  std::shared_ptr<LoadModel> model_base,
  std::set<ObjIDType> migratable_objects_here
) {
  auto const this_rank = vt::theContext()->getNode();
  vt_debug_print(
    terse, replay,
    "constructing load model to get loads from file location to home\n"
  );

  runInEpochCollective("WorkloadDataMigrator -> transferStatsHome", [&] {
    for (auto stat_obj_id : *model_base) {
      if (stat_obj_id.isMigratable()) {
        // if the object belongs here, do nothing; otherwise, "transfer" it to
        // the home rank so that it can later be sent to the rank holding the
        // object
        if (stat_obj_id.getHomeNode() != this_rank) {
          if (migratable_objects_here.count(stat_obj_id) == 0) {
            vt_debug_print(
              verbose, replay,
              "will transfer load of {} home to {}\n",
              stat_obj_id, stat_obj_id.getHomeNode()
            );
            migrateObjectTo(stat_obj_id, stat_obj_id.getHomeNode());
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
  auto const this_rank = vt::theContext()->getNode();
  vt_debug_print(
    terse, replay,
    "constructing load model to get loads from home to here\n"
  );

  runInEpochCollective("WorkloadDataMigrator -> transferStatsHere", [&] {
    for (auto stat_obj_id : migratable_objects_here) {
      // if the object is already here, do nothing; otherwise, "transfer" it
      // from the home rank so that we will have the needed workloads data
      bool workloads_here = false;
      for (auto other_id : *model_base) {
        if (stat_obj_id == other_id) {
          workloads_here = true;
          break;
        }
      }
      if (!workloads_here) {
        // check that this isn't something that should already have been here
        assert(stat_obj_id.getHomeNode() != this_rank);

        vt_debug_print(
          verbose, replay,
          "will transfer load of {} from home {}\n",
          stat_obj_id, stat_obj_id.getHomeNode()
        );
        ObjIDType mod_id = stat_obj_id;
        // Override curr_node to force retrieval from the home rank
        mod_id.curr_node = stat_obj_id.getHomeNode();
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

}}}} /* end namespace vt::vrt::collection::balance */
