/*
//@HEADER
// *****************************************************************************
//
//                                  baselb.cc
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_CC
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_CC

#include "vt/config.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/node_lb_data.h"
#include "vt/timing/timing.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/collective/collective_alg.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/phase/phase_manager.h"

#include <tuple>

namespace vt { namespace vrt { namespace collection { namespace lb {

std::shared_ptr<const balance::Reassignment> BaseLB::startLB(
  PhaseType phase,
  objgroup::proxy::Proxy<BaseLB> proxy,
  balance::LoadModel* model,
  StatisticMapType const& in_stats,
  ElementCommType const& in_comm_lb_data,
  TimeType total_load
) {
  start_time_ = timing::getCurrentTime();
  phase_ = phase;
  proxy_ = proxy;
  load_model_ = model;

  importProcessorData(in_stats, in_comm_lb_data);

  runInEpochCollective("BaseLB::startLB -> runLB", [this,total_load]{
    getArgs(phase_);
    inputParams(config_entry_.get());
    runLB(total_load);
  });

  return normalizeReassignments();
}

/*static*/
BaseLB::LoadType BaseLB::loadMilli(LoadType const& load) {
  // Convert `load` in seconds to milliseconds, typically for binning purposes
  return load * 1000;
}

void BaseLB::importProcessorData(
  StatisticMapType const& in_stats, ElementCommType const& comm_in
) {
  vt_debug_print(
    normal, lb,
    "importProcessorData: load data size={}, load comm size={}\n",
    load_model_->getNumObjects(), comm_in.size()
  );

  comm_data = &comm_in;
  base_stats_ = &in_stats;
}

void BaseLB::getArgs(PhaseType phase) {
  using namespace balance;

  bool has_config = ReadLBConfig::openConfig(theConfig()->vt_lb_file_name);
  if (has_config) {
    auto config = ReadLBConfig::entry(phase);
    if (config) {
      config_entry_ = std::make_unique<ConfigEntry>(*config);
    } else {
      vtAssert(false, "Error no config found, which must exist");
    }
  } else {
    auto const args = theConfig()->vt_lb_args;
    config_entry_ = std::make_unique<ConfigEntry>(
      ReadLBConfig::makeConfigFromParams(args)
    );
  }
}

std::shared_ptr<const balance::Reassignment> BaseLB::normalizeReassignments() {
  using namespace balance;

  auto this_node = theContext()->getNode();
  pending_reassignment_->node_ = this_node;

  runInEpochCollective("Sum migrations", [&] {
    auto cb = vt::theCB()->makeBcast<BaseLB, CountMsg, &BaseLB::finalize>(proxy_);
    int32_t local_migration_count = transfers_.size();
    auto msg = makeMessage<CountMsg>(local_migration_count);
    proxy_.template reduce<collective::PlusOp<int32_t>>(msg,cb);
  });

  std::map<NodeType, ObjDestinationListType> migrate_other;

  // Do local setup of reassignment data structure
  for (auto&& transfer : transfers_) {
    auto const obj_id = std::get<0>(transfer);
    auto const new_node = std::get<1>(transfer);
    auto const current_node = obj_id.curr_node;

    if (current_node == new_node) {
      vt_debug_print(
        verbose, lb, "BaseLB::normalizeReassignments(): self migration\n"
       );
    }

    // the object lives here, so it's departing.
    if (current_node == this_node) {
      pending_reassignment_->depart_[obj_id] = new_node;
    } else {
      // The user has specified a migration that the current host will
      // need to be informed of
      migrate_other[current_node].push_back(transfer);
    }
  }

  runInEpochCollective("BaseLB -> sendMigrateOthers", [&]{
    using ArriveListMsgType = TransferMsg<ObjDestinationListType>;

    for (auto&& other : migrate_other) {
      auto const current_host = std::get<0>(other);
      auto const& vec = std::get<1>(other);
      proxy_[current_host].template send<
        ArriveListMsgType, &BaseLB::notifyCurrentHostNodeOfObjectsDeparting
      >(vec);
    }
  });

  // At this point, all nodes should have complete data on which
  // objects will be departing, and to where

  // Do remote work to normalize the reassignments
  runInEpochCollective("BaseLB -> normalizeReassignments", [&]{
    // Notify all potential recipients for this reassignment that they have an
    // arriving object
    using DepartMsgType = TransferMsg<ObjLoadListType>;
    std::map<NodeType, ObjLoadListType> depart_map;

    for (auto&& departing : pending_reassignment_->depart_) {
      auto const obj_id = std::get<0>(departing);
      auto const dest = std::get<1>(departing);
      auto const load_summary = getObjectLoads(
        load_model_, obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      auto const raw_load_summary = getObjectRawLoads(
        load_model_, obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      depart_map[dest].push_back(
        std::make_tuple(obj_id, load_summary, raw_load_summary)
      );
    }

    for (auto&& depart_list : depart_map) {
      auto const dest = std::get<0>(depart_list);
      auto const& vec = std::get<1>(depart_list);
      proxy_[dest].template send<
        DepartMsgType, &BaseLB::notifyNewHostNodeOfObjectsArriving
      >(vec);
    }
  });

  // And now, all nodes should have complete data on which objects
  // will be arriving, and how much load they represent

  return pending_reassignment_;
}

void BaseLB::notifyCurrentHostNodeOfObjectsDeparting(
  TransferMsg<ObjDestinationListType>* msg
) {
  auto const& migrate_list = msg->getTransfer();
  for (auto&& obj : migrate_list) {
    pending_reassignment_->depart_[std::get<0>(obj)] = std::get<1>(obj);
  }
}

void BaseLB::notifyNewHostNodeOfObjectsArriving(
  TransferMsg<ObjLoadListType>* msg
) {
  auto const& arrival_list = msg->getTransfer();

  // Add arriving objects to our local reassignment list
  for (auto&& arrival : arrival_list) {
    auto const obj_id = std::get<0>(arrival);
    auto const& load_summary = std::get<1>(arrival);
    auto const& raw_load_summary = std::get<2>(arrival);
    pending_reassignment_->arrive_[obj_id] = std::make_tuple(
      load_summary, raw_load_summary
    );
  }
}

void BaseLB::migrateObjectTo(ObjIDType const obj_id, NodeType const to) {
  if (obj_id.curr_node != to || theConfig()->vt_lb_self_migration) {
    transfers_.push_back(TransferDestType{obj_id, to});
  }
}

void BaseLB::finalize(CountMsg* msg) {
  auto global_count = msg->getVal();
  if (migration_count_cb_) {
    migration_count_cb_(global_count);
  }

  pending_reassignment_->global_migration_count = global_count;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    TimeTypeWrapper const total_time = timing::getCurrentTime() - start_time_;
    vt_debug_print(
      terse, lb,
      "BaseLB::finalize: LB total time={}\n",
      total_time
    );
    fflush(stdout);
  }
}

void BaseLB::recvSharedEdges(CommMsg* msg) {
  auto phase = thePhase()->getCurrentPhase();
  auto comm_map = theNodeLBData()->getNodeComm(phase);

  if (comm_map != nullptr) {
    auto& comm = msg->comm_;
    for (auto&& elm : comm) {
      comm_map->insert(elm);
      vt_debug_print(
        verbose, lb, "recvSharedEdges: from={}, to={}\n",
        elm.first.fromObj(), elm.first.toObj()
      );
    }
  }
}

void BaseLB::setStrategySpecificModel(
  std::shared_ptr<balance::LoadModel> model
) {
  theLBManager()->setStrategySpecificModel(model);
}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_CC*/
