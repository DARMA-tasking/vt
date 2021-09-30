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
  ElementCommType const& in_comm_stats
) {
  start_time_ = timing::Timing::getCurrentTime();
  phase_ = phase;
  proxy_ = proxy;
  load_model_ = model;

  importProcessorData(in_stats, in_comm_stats);

  runInEpochCollective(
    "BaseLB::startLB -> finishedStats", [this]{ finishedStats(); }
  );
}

/*static*/
BaseLB::LoadType BaseLB::loadMilli(LoadType const& load) {
  // Convert `load` in seconds to milliseconds, typically for binning purposes
  return load * 1000;
}

BaseLB::ObjBinType BaseLB::histogramSample(LoadType const& load) const {
  auto const bin_size = getBinSize();
  ObjBinType const bin =
    ((static_cast<int32_t>(load)) / bin_size * bin_size)
    + bin_size;
  return bin;
}

void BaseLB::importProcessorData(
  StatisticMapType const& in_stats, ElementCommType const& comm_in
) {
  auto const& this_node = theContext()->getNode();
  vt_debug_print(
    normal, lb,
    "{}: importProcessorData: load stats size={}, load comm size={}\n",
    this_node, load_model_->getNumObjects(), comm_in.size()
  );
  for (auto obj : *load_model_) {
    auto load = load_model_->getWork(obj, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE});
    auto const& load_milli = loadMilli(load);
    auto const& bin = histogramSample(load_milli);
    this_load += load_milli;
    obj_sample[bin].push_back(obj);

    vt_debug_print(
      verbose, lb,
      "\t {}: importProcessorData: this_load={}, obj={}, home={}, load={}, "
      "load_milli={}, bin={}\n",
      this_node, this_load, obj.id, obj.home_node, load, load_milli, bin
    );
  }

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

  // Re-compute the statistics with the new partition based on current
  // this_load_ values
  // computeStatistics();
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

void BaseLB::finishedStats() {
  getArgs(phase_);
  this->inputParams(spec_entry_.get());
  this->runLB();
}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_CC*/
