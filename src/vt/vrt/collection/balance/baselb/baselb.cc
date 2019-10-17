/*
//@HEADER
// *****************************************************************************
//
//                                  baselb.cc
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_CC
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_CC

#include "vt/config.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/lb_comm.h"
#include "vt/vrt/collection/balance/lb_invoke/start_lb_msg.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/vrt/collection/balance/lb_invoke/invoke.h"
#include "vt/timing/timing.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/collective/collective_alg.h"
#include "vt/vrt/collection/balance/lb_common.h"

#include <tuple>

namespace vt { namespace vrt { namespace collection { namespace lb {

void BaseLB::startLBHandler(
  balance::StartLBMsg* msg, objgroup::proxy::Proxy<BaseLB> proxy
) {
  start_time_ = timing::Timing::getCurrentTime();
  phase_ = msg->getPhase();
  proxy_ = proxy;

  readLB(phase_);

  vtAssertExpr(balance::ProcStats::proc_data_.size() >= phase_);

  auto const& in_load_stats = balance::ProcStats::proc_data_[phase_];
  auto const& in_comm_stats = balance::ProcStats::proc_comm_[phase_];
  importProcessorData(in_load_stats, in_comm_stats);
  computeStatistics();
}

BaseLB::LoadType BaseLB::loadMilli(LoadType const& load) const {
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
  ElementLoadType const& load_in, ElementCommType const& comm_in
) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    lb, node,
    "{}: importProcessorData: load stats size={}, load comm size={}\n",
    this_node, load_in.size(), comm_in.size()
  );
  for (auto&& stat : load_in) {
    auto const& obj = stat.first;
    auto const& load = stat.second;
    auto const& load_milli = loadMilli(load);
    auto const& bin = histogramSample(load_milli);
    this_load += load_milli;
    obj_sample[bin].push_back(obj);

    debug_print_verbose(
      lb, node,
      "\t {}: importProcessorData: this_load={}, obj={}, load={}, "
      "load_milli={}, bin={}\n",
      this_node, this_load, obj, load, load_milli, bin
    );
  }

  load_data = &load_in;
  comm_data = &comm_in;
}

void BaseLB::readLB(PhaseType phase) {
  using namespace balance;
  ReadLBSpec::openFile();
  ReadLBSpec::readFile();

  bool fallback = true;
  bool has_spec = ReadLBSpec::hasSpec();
  if (has_spec) {
    auto spec = ReadLBSpec::entry(phase);
    if (spec) {
      bool has_min_only = false;
      if (spec->hasMin()) {
        min_threshold = spec->min();
        has_min_only = true;
      }
      if (spec->hasMax()) {
        max_threshold = spec->max();
        has_min_only = false;
      }
      if (has_min_only) {
        auto_threshold = false;
      }
      fallback = false;
    }
  }

  if (fallback) {
    max_threshold  = this->getDefaultMaxThreshold();
    min_threshold  = this->getDefaultMinThreshold();
    auto_threshold = this->getDefaultAutoThreshold();
  }
}

void BaseLB::statsHandler(StatsMsgType* msg) {
  auto in       = msg->getConstVal();
  auto max      = in.max();
  auto min      = in.min();
  auto avg      = in.avg();
  auto sum      = in.sum();
  auto npr      = in.npr();
  auto car      = in.N_;
  auto imb      = in.I();
  auto var      = in.var();
  auto stdv     = in.stdv();
  auto skew     = in.skew();
  auto krte     = in.krte();
  auto the_stat = msg->stat_;

  stats[the_stat][lb::StatisticQuantity::max] = max;
  stats[the_stat][lb::StatisticQuantity::min] = min;
  stats[the_stat][lb::StatisticQuantity::avg] = avg;
  stats[the_stat][lb::StatisticQuantity::sum] = sum;
  stats[the_stat][lb::StatisticQuantity::npr] = npr;
  stats[the_stat][lb::StatisticQuantity::car] = car;
  stats[the_stat][lb::StatisticQuantity::var] = var;
  stats[the_stat][lb::StatisticQuantity::npr] = npr;
  stats[the_stat][lb::StatisticQuantity::imb] = imb;
  stats[the_stat][lb::StatisticQuantity::std] = stdv;
  stats[the_stat][lb::StatisticQuantity::skw] = skew;
  stats[the_stat][lb::StatisticQuantity::kur] = krte;

  if (theContext()->getNode() == 0) {
    vt_print(
      lb,
      "BaseLB: Statistic={}: "
      " max={:.2f}, min={:.2f}, sum={:.2f}, avg={:.2f}, var={:.2f},"
      " stdev={:.2f}, nproc={}, cardinality={} skewness={:.2f}, kurtosis={:.2f},"
      " npr={}, imb={:.2f}, num_stats={}\n",
      lb_stat_name_[the_stat],
      max, min, sum, avg, var, stdv, npr, car, skew, krte, npr, imb,
      stats.size()
    );
  }

  if (stats.size() == static_cast<std::size_t>(num_reduce_stats_)) {
    finishedStats();
  }
}

EpochType BaseLB::getMigrationEpoch() const {
  return migration_epoch_;
}

EpochType BaseLB::startMigrationCollective() {
  migration_epoch_ = theTerm()->makeEpochCollective();
  theTerm()->addAction(migration_epoch_, [this]{ this->migrationDone(); });
  theMsg()->pushEpoch(migration_epoch_);
  return migration_epoch_;
}

void BaseLB::finishMigrationCollective() {
  for (auto&& elm : off_node_migrate_) {
    transferSend(elm.first, elm.second, migration_epoch_);
  }

  off_node_migrate_.clear();

  theMsg()->popEpoch(migration_epoch_);
  theTerm()->finishedEpoch(migration_epoch_);
}

void BaseLB::transferSend(
  NodeType from, TransferVecType const& transfer, EpochType epoch
) {
  using MsgType = TransferMsg<TransferVecType>;
  auto msg = makeMessage<MsgType>(transfer);
  envelopeSetEpoch(msg->env, epoch);
  proxy_[from].template send<MsgType,&BaseLB::transferMigrations>(msg);
}

void BaseLB::transferMigrations(TransferMsg<TransferVecType>* msg) {
  auto const& migrate_list = msg->getTransfer();
  for (auto&& elm : migrate_list) {
    auto obj_id  = std::get<0>(elm);
    auto to_node = std::get<1>(elm);
    vtAssertExpr(
      balance::ProcStats::proc_migrate_.find(obj_id) !=
      balance::ProcStats::proc_migrate_.end()
    );
    migrateObjectTo(obj_id, to_node);
  }
}

void BaseLB::migrateObjectTo(ObjIDType const obj_id, NodeType const to) {
  auto& migrator = balance::ProcStats::proc_migrate_;
  auto iter = migrator.find(obj_id);
  auto from = objGetNode(obj_id);

  debug_print(
    lb, node,
    "migrateObjectTo, obj_id={}, from={}, to={}, found={}\n",
    obj_id, from, to, iter != migrator.end()
  );

  local_migration_count_++;

  if (iter == migrator.end()) {
    off_node_migrate_[from].push_back(std::make_tuple(obj_id,to));
  } else {
    iter->second(to);
  }
}

void BaseLB::finalize(CountMsg* msg) {
  auto global_count = msg->getVal();
  auto const& this_node = theContext()->getNode();
  debug_print(
    lb, node,
    "BaseLB::finalize: finished migrations: local migration count={}\n",
    local_migration_count_
  );
  if (this_node == 0) {
    auto const total_time = timing::Timing::getCurrentTime() - start_time_;
    vt_print(
      lb,
      "BaseLB::finalize: LB total time={}, total migration count={}\n",
      total_time, global_count
    );
    fflush(stdout);
  }
  balance::ProcStats::startIterCleanup();
  balance::LBManager::finishedRunningLB(phase_);
}

void BaseLB::migrationDone() {
  debug_print(
    lb, node,
    "BaseLB::migrationDone: start reduce local migration count={}\n",
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
  this->runLB();
}

void BaseLB::computeStatistics() {
  debug_print(
    lb, node,
    "computeStatistics: this_load={}\n", this_load
  );

  computeStatisticsOver(Statistic::P_l);
  computeStatisticsOver(Statistic::O_l);

  if (comm_aware_) {
    computeStatisticsOver(Statistic::P_c);
    computeStatisticsOver(Statistic::O_c);
  }
  // @todo: add P_c, P_t, O_c, O_t
}

balance::LoadData BaseLB::reduceVec(std::vector<balance::LoadData>&& vec) const {
  balance::LoadData reduce_ld(0.0f);
  if (vec.size() == 0) {
    return reduce_ld;
  } else {
    for (std::size_t i = 1; i < vec.size(); i++) {
      vec[0] = vec[0] + vec[i];
    }
    return vec[0];
  }
}

bool BaseLB::isCollectiveComm(balance::CommCategory cat) const {
  bool is_collective =
    cat == balance::CommCategory::Broadcast or
    cat == balance::CommCategory::CollectionToNodeBcast or
    cat == balance::CommCategory::NodeToCollectionBcast;
  return is_collective;
}

void BaseLB::computeStatisticsOver(Statistic stat) {
  using ReduceOp = collective::PlusOp<balance::LoadData>;

  num_reduce_stats_++;

  auto cb = vt::theCB()->makeBcast<BaseLB, StatsMsgType, &BaseLB::statsHandler>(proxy_);

  switch (stat) {
  case Statistic::P_l: {
    // Perform the reduction for P_l -> processor load only
    auto msg = makeMessage<StatsMsgType>(Statistic::P_l, this_load);
    proxy_.template reduce<ReduceOp>(msg,cb);
  }
  break;
  case Statistic::P_c: {
    // Perform the reduction for P_c -> processor comm only
    double comm_load = 0.0;
    for (auto&& elm : *comm_data) {
      if (not comm_collectives_ and isCollectiveComm(elm.first.cat_)) {
        continue;
      }
      if (elm.first.onNode() or elm.first.selfEdge()) {
        continue;
      }
      //vt_print(lb, "comm_load={}, elm={}\n", comm_load, elm.second);
      comm_load += elm.second;
    }
    auto msg = makeMessage<StatsMsgType>(Statistic::P_c, comm_load);
    proxy_.template reduce<ReduceOp>(msg,cb);
  }
  break;
  case Statistic::O_c: {
    // Perform the reduction for O_c -> object comm only
    std::vector<balance::LoadData> lds;
    for (auto&& elm : *comm_data) {
      // Only count object-to-object direct edges in the O_c statistics
      if (elm.first.cat_ == balance::CommCategory::SendRecv and not elm.first.selfEdge()) {
        lds.emplace_back(balance::LoadData(elm.second));
      }
    }
    auto msg = makeMessage<StatsMsgType>(Statistic::O_c, reduceVec(std::move(lds)));
    proxy_.template reduce<ReduceOp>(msg,cb);
  }
  break;
  case Statistic::O_l: {
    // Perform the reduction for O_l -> object load only
    std::vector<balance::LoadData> lds;
    for (auto&& elm : *load_data) {
      lds.emplace_back(balance::LoadData(elm.second));
    }
    auto msg = makeMessage<StatsMsgType>(Statistic::O_l, reduceVec(std::move(lds)));
    proxy_.template reduce<ReduceOp>(msg,cb);
  }
  break;
  default:
    break;
  }

}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_CC*/
