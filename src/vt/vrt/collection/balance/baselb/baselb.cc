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
#include "vt/vrt/collection/balance/lb_comm.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/node_stats.h"
#include "vt/timing/timing.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/collective/collective_alg.h"
#include "vt/vrt/collection/balance/lb_common.h"

#include <tuple>

namespace vt { namespace vrt { namespace collection { namespace lb {

void BaseLB::startLB(
  PhaseType phase,
  objgroup::proxy::Proxy<BaseLB> proxy,
  balance::LoadModel* model,
  StatisticMapType const& in_stats,
  ElementCommType const& in_comm_stats,
  TimeType total_load
) {
  start_time_ = timing::Timing::getCurrentTime();
  phase_ = phase;
  proxy_ = proxy;
  load_model_ = model;

  importProcessorData(in_stats, in_comm_stats);

  runInEpochCollective(
    "BaseLB::startLB -> runLB", [this,total_load]{
      getArgs(phase_);
      inputParams(spec_entry_.get());
      runLB(total_load);
    }
  );
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
    "importProcessorData: load stats size={}, load comm size={}\n",
    load_model_->getNumObjects(), comm_in.size()
  );

  comm_data = &comm_in;
  base_stats_ = &in_stats;
}

void BaseLB::getArgs(PhaseType phase) {
  using namespace balance;

  bool has_spec = ReadLBSpec::openSpec(theConfig()->vt_lb_file_name);
  if (has_spec) {
    auto spec = ReadLBSpec::entry(phase);
    if (spec) {
      spec_entry_ = std::make_unique<SpecEntry>(*spec);
    } else {
      vtAssert(false, "Error no spec found, which must exist");
    }
  } else {
    auto const args = theConfig()->vt_lb_args;
    spec_entry_ = std::make_unique<SpecEntry>(
      ReadLBSpec::makeSpecFromParams(args)
    );
  }
}

void BaseLB::applyMigrations(
  TransferVecType const &transfers, MigrationCountCB migration_count_callback
) {
  migration_count_cb_ = migration_count_callback;

  TransferType off_node_migrate;

  for (auto&& elm : transfers) {
    auto obj_id = std::get<0>(elm);
    auto to = std::get<1>(elm);
    auto from = objGetNode(obj_id);

    if (from != to) {
      bool has_object = theNodeStats()->hasObjectToMigrate(obj_id);

      vt_debug_print(
        normal, lb,
        "migrateObjectTo, obj_id={}, home={}, from={}, to={}, found={}\n",
        obj_id.id, obj_id.home_node, from, to, has_object
      );

      local_migration_count_++;
      if (has_object) {
        theNodeStats()->migrateObjTo(obj_id, to);
      } else {
        off_node_migrate[from].push_back(std::make_tuple(obj_id,to));
      }
    }
  }

  for (auto&& elm : off_node_migrate) {
    transferSend(elm.first, elm.second);
  }

  // @todo: removed re-computation of statistics, which needs to be added back
  // in the future.
  migrationDone();
}

void BaseLB::transferSend(NodeType from, TransferVecType const& transfer) {
  using MsgType = TransferMsg<TransferVecType>;
  proxy_[from].template send<MsgType,&BaseLB::transferMigrations>(transfer);
}

void BaseLB::transferMigrations(TransferMsg<TransferVecType>* msg) {
  auto const& migrate_list = msg->getTransfer();
  for (auto&& elm : migrate_list) {
    auto obj_id  = std::get<0>(elm);
    auto to_node = std::get<1>(elm);
    vtAssert(theNodeStats()->hasObjectToMigrate(obj_id), "Must have object");
    theNodeStats()->migrateObjTo(obj_id, to_node);
  }
}

std::unique_ptr<balance::Reassignment> BaseLB::normalizeReassignments() {
  using namespace balance;

  Reassignment& r = *pending_reassignment_.get();

  r.node_ = theContext()->getNode();

  // Do local setup of reassignment data structure
  for (auto&& transfer : transfers_) {
    auto const obj_id = std::get<0>(transfer);
    auto const new_node = std::get<1>(transfer);
    auto const current_node = obj_id.curr_node;

    // self-migration
    if (current_node == new_node) {
      vtAbort("Not currently implemented -- self-migration");
    }

    // the object lives here, so it's departing.
    if (current_node == r.node_) {
      r.depart_[obj_id] = new_node;
    } else if (new_node == r.node_) {
      // the object's new location is here---so it's arriving---but we don't
      // have the data most likely for it, so we will receive it later
      r.arrive_[obj_id] = {};
    } else {
      // The user has specified a migration neither on the send or receive side?
      vtAbort(
        "A migration has been requested for an object that neither lives here"
        " nor will be migrated here"
      );
    }
  }

  // Do remote work to normalize the reassignments
  runInEpochCollective("BaseLB -> normalizeReassignments", [&]{
    // Notify all potential recipients for this reassignment that they have an
    // arriving object
    using DepartMsgType = TransferMsg<DepartListType>;
    std::map<NodeType, DepartListType> depart_map;

    for (auto&& departing : r.depart_) {
      auto const obj_id = std::get<0>(departing);
      auto const dest = std::get<1>(departing);
      auto const load_summary = getObjectLoads(
        load_model_, obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      depart_map[dest].push_back(std::make_tuple(obj_id, load_summary));
    }

    for (auto&& depart_list : depart_map) {
      auto const dest = std::get<0>(depart_list);
      auto const& vec = std::get<1>(depart_list);
      proxy_[dest].template send<DepartMsgType, &BaseLB::notifyDeparting>(vec);
    }

    // Notify current owners of objects that their object has been reassigned
    // to this node
    using ArriveMsgType = TransferMsg<ArriveListType>;
    std::map<NodeType, ArriveListType> arrive_map;

    for (auto&& arriving : r.arrive_) {
      auto const obj_id = std::get<0>(arriving);
      auto const current_node = obj_id.curr_node;
      arrive_map[current_node].push_back(std::make_tuple(obj_id, r.node_));
    }

    for (auto&& arrive_list : arrive_map) {
      auto const dest = std::get<0>(arrive_list);
      auto const& vec = std::get<1>(arrive_list);
      proxy_[dest].template send<ArriveMsgType, &BaseLB::notifyArriving>(vec);
    }
  });

  return std::move(pending_reassignment_);
}

void BaseLB::notifyDeparting(TransferMsg<DepartListType>* msg) {
  auto const& arrival_list = msg->getTransfer();

  // Add arriving objects to our local reassignment list
  for (auto&& arrival : arrival_list) {
    auto const obj_id = std::get<0>(arrival);
    auto const& load_summary = std::get<1>(arrival);
    pending_reassignment_->arrive_[obj_id] = load_summary;
  }
}

void BaseLB::notifyArriving(TransferMsg<ArriveListType>* msg) {
  using namespace balance;

  auto const& depart_list = msg->getTransfer();
  auto& r = *pending_reassignment_.get();

  NodeType from = uninitialized_destination;
  DepartListType summary;

  // Add departing objects to our local reassignment list
  for (auto&& depart : depart_list) {
    auto const obj_id = std::get<0>(depart);
    auto const& new_node = std::get<1>(depart);
    r.depart_[obj_id] = new_node;
    from = obj_id.curr_node;
    summary.push_back(
      std::make_tuple(obj_id, getObjectLoads(
        load_model_, obj_id, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
     )
    ));
  }

  using LoadSummaryMsg = TransferMsg<DepartListType>;
  proxy_[from].template send<LoadSummaryMsg, &BaseLB::arriveLoadSummary>(summary);
}

void BaseLB::arriveLoadSummary(TransferMsg<DepartListType>* msg) {
  auto& r = *pending_reassignment_.get();
  auto const& arrive_list = msg->getTransfer();
  for (auto&& elm : arrive_list) {
    auto const obj_id = std::get<0>(elm);
    auto const load_sum = std::get<1>(elm);
    r.arrive_[obj_id] = load_sum;
  }
}

void BaseLB::migrateObjectTo(ObjIDType const obj_id, NodeType const to) {
  transfers_.push_back(TransferDestType{obj_id, to});
}

void BaseLB::finalize(CountMsg* msg) {
  auto global_count = msg->getVal();
  if (migration_count_cb_) {
    migration_count_cb_(global_count);
  }
  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const total_time = timing::Timing::getCurrentTime() - start_time_;
    vt_print(
      lb,
      "BaseLB::finalize: LB total time={}, total migration count={}\n",
      total_time, global_count
    );
    fflush(stdout);
  }
}

void BaseLB::migrationDone() {
  vt_debug_print(
    normal, lb,
    "BaseLB::migrationDone: local migration count={}\n",
    local_migration_count_
  );
  auto cb = vt::theCB()->makeBcast<BaseLB, CountMsg, &BaseLB::finalize>(proxy_);
  auto msg = makeMessage<CountMsg>(local_migration_count_);
  proxy_.template reduce<collective::PlusOp<int32_t>>(msg,cb);
}

NodeType BaseLB::objGetNode(ObjIDType const id) const {
  return balance::objGetNode(id);
}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_CC*/
