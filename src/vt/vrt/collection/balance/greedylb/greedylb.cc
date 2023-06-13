/*
//@HEADER
// *****************************************************************************
//
//                                 greedylb.cc
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC

#include "vt/config.h"
#include "vt/vrt/collection/balance/greedylb/greedylb.h"
#include "vt/vrt/collection/balance/greedylb/greedylb.fwd.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_types.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_constants.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_msgs.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/serialization/messaging/serialized_messenger.h"
#include "vt/context/context.h"
#include "vt/vrt/collection/manager.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/vrt/collection/balance/lb_args_enum_converter.h"
#include "vt/timing/timing.h"

#include <unordered_map>
#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace lb {

/*static*/ objgroup::proxy::Proxy<GreedyLB> GreedyLB::scatter_proxy = {};

void GreedyLB::init(objgroup::proxy::Proxy<GreedyLB> in_proxy) {
  proxy = scatter_proxy = in_proxy;
}

/*static*/ std::unordered_map<std::string, std::string>
GreedyLB::getInputKeysWithHelp() {
  std::unordered_map<std::string, std::string> const keys_help = {
    {
      "min",
      R"(
Values: <double>
Default: 0.8
Description:
  The load threshold of objects to consider for potential migration on each
  rank. All objects over threshold * average_load on each rank will be
  considered. If the parameter "auto" is set to "true", this will be the minimum
  threshold; otherwise, it sets the threshold directly.
)"
    },
    {
      "max",
      R"(
Values: <double>
Default: 1.004
Description:
  The maximum load threshold for objects to consider on each node which is only
  used if "auto" is "true".
)"
    },
    {
      "auto",
      R"(
Values: {true, false}
Default: true
Description:
  Automatically determine the threshold between "min" and "max" using
  calculated I (imbalance metric) with the formula
  min(max(1-I, min), max).
)"
    },
    {
      "strategy",
      R"(
Values: {scatter, bcast, pt2pt}
Default: scatter
Description:
  How to distribute the data after the centralized LB makes a decision
)"
    }
  };
  return keys_help;
}

void GreedyLB::inputParams(balance::ConfigEntry* config) {
  auto keys_help = getInputKeysWithHelp();

  std::vector<std::string> allowed;
  for (auto&& elm : keys_help) {
    allowed.push_back(elm.first);
  }
  config->checkAllowedKeys(allowed);
  min_threshold = config->getOrDefault<double>("min", greedy_threshold_p);
  max_threshold = config->getOrDefault<double>("max", greedy_max_threshold_p);
  auto_threshold = config->getOrDefault<bool>("auto", greedy_auto_threshold_p);

  balance::LBArgsEnumConverter<DataDistStrategy> strategy_converter_(
    "strategy", "DataDistStrategy", {
      {DataDistStrategy::scatter, "scatter"},
      {DataDistStrategy::pt2pt,   "pt2pt"},
      {DataDistStrategy::bcast,   "bcast"}
    }
  );
  strat_ = strategy_converter_.getFromConfig(config, strat_);
}

void GreedyLB::runLB(TimeType total_load) {
  this_load = loadMilli(total_load);
  buildHistogram();
  loadStats();
}

void GreedyLB::loadStats() {
  auto const& this_node = theContext()->getNode();
  auto avg_load = getAvgLoad();
  auto total_load = getSumLoad();
  auto I = getStats()->at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::imb
  );

  bool should_lb = false;
  this_load_begin = this_load;

  if (avg_load > 0.0000000001) {
    should_lb = I > greedy_tolerance;
  }

  if (auto_threshold) {
    this_threshold = std::min(std::max(1.0f - I, min_threshold), max_threshold);
  }

  if (this_node == 0) {
    vt_debug_print(
      terse, lb,
      "loadStats: load={}, total={}, avg={}, I={:.2f},"
      "should_lb={}, auto={}, threshold={}\n",
      TimeTypeWrapper(this_load / 1000), TimeTypeWrapper(total_load / 1000),
      TimeTypeWrapper(avg_load / 1000), I, should_lb, auto_threshold,
      TimeTypeWrapper(this_threshold / 1000)
    );
    if (!should_lb) {
      vt_print(
        lb,
        "GreedyLB decided to skip rebalancing due to low imbalance\n"
      );
    }
    fflush(stdout);
  }

  if (should_lb) {
    calcLoadOver();
    reduceCollect();
  }
}

void GreedyLB::collectHandler(GreedyPayload payload) {
  vt_debug_print(
    normal, lb,
    "GreedyLB::collectHandler: entries size={}\n",
    payload.getSample().size()
  );

  for (auto&& elm : payload.getSample()) {
    vt_debug_print(
      verbose, lb,
      "\t collectHandler: bin={}, num={}\n",
      elm.first, elm.second.size()
    );
  }

  auto objs = std::move(payload.getSampleMove());
  auto profile = std::move(payload.getLoadProfileMove());
  runBalancer(std::move(objs),std::move(profile));
}

void GreedyLB::reduceCollect() {
  vt_debug_print(
    verbose, lb,
    "GreedyLB::reduceCollect: load={}, load_begin={} load_over.size()={}\n",
    TimeTypeWrapper(this_load / 1000),
    TimeTypeWrapper(this_load_begin / 1000), load_over.size()
  );
  proxy.reduce<&GreedyLB::collectHandler, collective::PlusOp>(
    proxy[0], GreedyPayload{load_over, this_load}
  );
}

void GreedyLB::runBalancer(
  ObjSampleType&& in_objs, LoadProfileType&& in_profile
) {
  using CompRecType = GreedyCompareLoadMax<GreedyRecord>;
  using CompProcType = GreedyCompareLoadMin<GreedyProc>;
  auto const& num_nodes = theContext()->getNumNodes();
  ObjSampleType objs{std::move(in_objs)};
  LoadProfileType profile{std::move(in_profile)};
  std::vector<GreedyRecord> recs;
  vt_debug_print(
    normal, lb,
    "GreedyLB::runBalancer: objs={}, profile={}\n",
    objs.size(), profile.size()
  );
  for (auto&& elm : objs) {
    auto const& bin = elm.first;
    auto const& obj_list = elm.second;
    for (auto&& obj : obj_list) {
      recs.emplace_back(GreedyRecord{obj,static_cast<LoadType>(bin)});
    }
  }
  std::make_heap(recs.begin(), recs.end(), CompRecType());
  auto nodes = std::vector<GreedyProc>{};
  for (NodeType n = 0; n < num_nodes; n++) {
    auto iter = profile.find(n);
    vtAssert(iter != profile.end(), "Must have load profile");
    nodes.emplace_back(GreedyProc{n,iter->second});
    vt_debug_print(
      verbose, lb,
      "\t GreedyLB::runBalancer: node={}, profile={}\n",
      n, iter->second
    );
  }
  std::make_heap(nodes.begin(), nodes.end(), CompProcType());
  auto lb_size = recs.size();
  for (size_t i = 0; i < lb_size; i++) {
    std::pop_heap(recs.begin(), recs.end(), CompRecType());
    auto max_rec = recs.back();
    recs.pop_back();
    std::pop_heap(nodes.begin(), nodes.end(), CompProcType());
    auto min_node = nodes.back();
    nodes.pop_back();
    vt_debug_print(
      verbose, lb,
      "\t GreedyLB::runBalancer: min_node={}, load_={}, "
      "recs_={}, max_rec: obj={}, time={}\n",
      min_node.node_, TimeTypeWrapper(min_node.load_ / 1000),
      min_node.recs_.size(), max_rec.getObj(),
      TimeTypeWrapper(max_rec.getModeledLoad() / 1000)
    );
    min_node.recs_.push_back(max_rec.getObj());
    min_node.load_ += max_rec.getModeledLoad();
    nodes.push_back(min_node);
    std::push_heap(nodes.begin(), nodes.end(), CompProcType());
  }
  return transferObjs(std::move(nodes));
}

GreedyLB::ObjIDType GreedyLB::objSetNode(
  NodeType const& node, ObjIDType const& id
) {
  auto new_id = id;
  new_id.curr_node = node;
  return new_id;
}

void GreedyLB::recvObjs(GreedySendMsg* msg) {
  vt_debug_print(
    normal, lb,
    "recvObjs: msg->transfer_.size={}\n", msg->transfer_.size()
  );
  recvObjsDirect(msg->transfer_.size(), msg->transfer_.data());
}

void GreedyLB::recvObjsBcast(GreedyBcastMsg* msg) {
  auto const n = theContext()->getNode();
  vt_debug_print(
    normal, lb,
    "recvObjs: msg->transfer_.size={}\n", msg->transfer_[n].size()
  );
  recvObjsDirect(msg->transfer_[n].size(), msg->transfer_[n].data());
}

void GreedyLB::recvObjsDirect(std::size_t len, GreedyLBTypes::ObjIDType* objs) {
  auto const& this_node = theContext()->getNode();
  auto const& num_recs = len;
  vt_debug_print(
    normal, lb,
    "recvObjsDirect: num_recs={}\n", num_recs
  );

  for (std::size_t i = 0; i < len; i++) {
    auto const to_node = objGetNode(objs[i]);
    auto const new_obj_id = objSetNode(this_node,objs[i]);
    vt_debug_print(
      verbose, lb,
      "\t recvObjs: i={}, to_node={}, obj={}, new_obj_id={}, num_recs={}"
      "\n",
      i, to_node, objs[i], new_obj_id, num_recs
    );

    migrateObjectTo(new_obj_id, to_node);
  }
}

/*static*/ void GreedyLB::recvObjsHan(GreedyLBTypes::ObjIDType* objs) {
  vt_debug_print(
    verbose, lb,
    "recvObjsHan: num_recs={}\n", *objs
  );
  scatter_proxy.get()->recvObjsDirect(static_cast<std::size_t>(objs->id), objs+1);
}

void GreedyLB::transferObjs(std::vector<GreedyProc>&& in_load) {
  std::size_t max_recs = 1;
  std::vector<GreedyProc> load(std::move(in_load));
  std::vector<std::vector<GreedyLBTypes::ObjIDType>> node_transfer(load.size());
  for (auto&& elm : load) {
    auto const& node = elm.node_;
    auto const& recs = elm.recs_;
    for (auto&& rec : recs) {
      auto const cur_node = objGetNode(rec);
      // transfer required from `cur_node' to `node'
      if (cur_node != node) {
        auto const new_obj_id = objSetNode(node, rec);
        node_transfer[cur_node].push_back(new_obj_id);
        max_recs = std::max(max_recs, node_transfer[cur_node].size() + 1);
      }
    }
  }

  if (strat_ == DataDistStrategy::scatter) {
    std::size_t max_bytes =  max_recs * sizeof(GreedyLBTypes::ObjIDType);
    vt_debug_print(
      normal, lb,
      "GreedyLB::transferObjs: max_recs={}, max_bytes={}\n",
      max_recs, max_bytes
    );
    theCollective()->scatter<GreedyLBTypes::ObjIDType,recvObjsHan>(
      max_bytes*load.size(),max_bytes,nullptr,[&](NodeType node, void* ptr){
        auto ptr_out = reinterpret_cast<GreedyLBTypes::ObjIDType*>(ptr);
        auto const& proc = node_transfer[node];
        auto const& rec_size = proc.size();
        ptr_out->id = rec_size;
        for (size_t i = 0; i < rec_size; i++) {
          *(ptr_out + i + 1) = proc[i];
        }
      }
    );
  } else if (strat_ == DataDistStrategy::pt2pt) {
    for (NodeType n = 0; n < theContext()->getNumNodes(); n++) {
      vtAssert(
        node_transfer.size() == static_cast<size_t>(theContext()->getNumNodes()),
        "Must contain all nodes"
      );
      proxy[n].send<GreedySendMsg, &GreedyLB::recvObjs>(node_transfer[n]);
    }
  } else if (strat_ == DataDistStrategy::bcast) {
    proxy.broadcast<GreedyBcastMsg, &GreedyLB::recvObjsBcast>(node_transfer);
  }
}

double GreedyLB::getAvgLoad() const {
  return getStats()->at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::avg
  );
}

double GreedyLB::getMaxLoad() const {
  return getStats()->at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::max
  );
}

double GreedyLB::getSumLoad() const {
  return getStats()->at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::sum
  );
}

void GreedyLB::loadOverBin(ObjBinType bin, ObjBinListType& bin_list) {
  auto avg_load = getAvgLoad();
  auto const threshold = this_threshold * avg_load;
  auto const obj_id = bin_list.back();

  load_over[bin].push_back(obj_id);
  bin_list.pop_back();

  auto const& obj_time_milli = loadMilli(load_model_->getModeledLoad(
    obj_id, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}
  ));

  this_load -= obj_time_milli;

  vt_debug_print(
    normal, lb,
    "loadOverBin: this_load_begin={}, this_load={}, threshold={}: "
    "adding unit: bin={}, milli={}\n",
    TimeTypeWrapper(this_load_begin / 1000),
    TimeTypeWrapper(this_load / 1000), TimeTypeWrapper(threshold / 1000),
    bin, obj_time_milli
  );
}

void GreedyLB::calcLoadOver() {
  auto avg_load = getAvgLoad();
  auto const threshold = this_threshold * avg_load;

  vt_debug_print(
    normal, lb,
    "calcLoadOver: this_load={}, avg_load={}, threshold={}\n",
    TimeTypeWrapper(this_load / 1000),
    TimeTypeWrapper(avg_load / 1000),
    TimeTypeWrapper(threshold / 1000)
  );

  auto cur_item = obj_sample.begin();
  while (this_load > threshold && cur_item != obj_sample.end()) {
    if (cur_item->second.size() != 0) {
      loadOverBin(cur_item->first, cur_item->second);
    } else {
      cur_item++;
    }
  }

  for (size_t i = 0; i < obj_sample.size(); i++) {
    auto obj_iter = obj_sample.find(i);
    if (obj_iter != obj_sample.end() && obj_iter->second.size() == 0) {
      obj_sample.erase(obj_iter);
    }
  }
}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC*/
