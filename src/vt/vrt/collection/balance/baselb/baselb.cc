/*
//@HEADER
// ************************************************************************
//
//                          baselb.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
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

BaseLB::LoadType BaseLB::loadMilli(LoadType const& load) {
  return load * 1000;
}

BaseLB::ObjBinType BaseLB::histogramSample(LoadType const& load) {
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

    debug_print(
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
  auto var      = in.variance();
  auto stdev    = in.stdev();
  auto skewness = in.skewness();
  auto kurtosis = in.kurtosis();
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
  stats[the_stat][lb::StatisticQuantity::std] = stdev;
  stats[the_stat][lb::StatisticQuantity::skw] = skewness;
  stats[the_stat][lb::StatisticQuantity::kur] = kurtosis;

  fmt::print(
    "statsHandler:"
    " max={}, min={}, sum={}, avg={}, var={}, stdev={}, nproc={}, cardinality={} "
    "skewness={}, kurtosis={}, npr={}, imb={}, num_stats={}\n",
    max, min, sum, avg, var, stdev, npr, car, skewness, kurtosis, npr, imb,
    stats.size()
  );

  if (stats.size() == 1) {
    finishedStats();
  }
}

EpochType BaseLB::getMigrationEpoch() const {
  return migration_epoch_;
}

EpochType BaseLB::startMigrationCollective() {
  migration_epoch_ = theTerm()->makeEpochCollective();
  theTerm()->addAction(migration_epoch_, [this]{ this->migrationDone(); });
  return migration_epoch_;
}

void BaseLB::finishMigrationCollective() {
  for (auto&& elm : off_node_migrate_) {
    transferSend(elm.first, elm.second, migration_epoch_);
  }

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
  vtAssert(migration_epoch_ != no_epoch, "Must call startMigrationCollective");

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
    "finished all object migrations: total migration count={}\n",
    global_count
  );
  if (this_node == 0) {
    auto const total_time = timing::Timing::getCurrentTime() - start_time_;
    vt_print(
      lb,
      "migrationDone: LB total time={}, local migration count={}\n",
      total_time, local_migration_count_
    );
    fflush(stdout);
  }
  balance::ProcStats::startIterCleanup();
  balance::InvokeLB::releaseLBCollective(phase_);
}

void BaseLB::migrationDone() {
  auto cb = vt::theCB()->makeBcast<BaseLB, CountMsg, &BaseLB::finalize>(proxy_);
  auto msg = makeMessage<CountMsg>(local_migration_count_);
  proxy_.template reduce<collective::PlusOp<int32_t>>(msg,cb);
}

NodeType BaseLB::objGetNode(ObjIDType const id) {
  return id & 0x0000000FFFFFFFF;
}

void BaseLB::finishedStats() {
  this->runLB();
}

void BaseLB::computeStatistics() {
  using ReduceOp = collective::PlusOp<balance::LoadData>;

  debug_print(
    lb, node,
    "computeStatistics: this_load={}\n", this_load
  );

  // Perform the reduction for W_l -> load only
  auto cb = vt::theCB()->makeBcast<BaseLB, StatsMsgType, &BaseLB::statsHandler>(proxy_);
  auto msg = makeMessage<StatsMsgType>(Statistic::W_l, this_load);
  proxy_.template reduce<ReduceOp>(msg,cb);

  // @todo: add W_c, and W_t
}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_BASELB_CC*/
