/*
//@HEADER
// *****************************************************************************
//
//                                 charmlb.cc
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

#include "vt/vrt/collection/balance/charmlb/TreeStrategyBase.h"
#include "vt/vrt/collection/balance/lb_common.h"
#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_CHARMLB_CHARMLB_CC
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_CHARMLB_CHARMLB_CC

#include "vt/config.h"
#include "vt/vrt/collection/balance/charmlb/charmlb.h"
#include "vt/vrt/collection/balance/charmlb/charmlb.fwd.h"
#include "vt/vrt/collection/balance/charmlb/charmlb_types.h"
#include "vt/vrt/collection/balance/charmlb/charmlb_constants.h"
#include "vt/vrt/collection/balance/charmlb/charmlb_msgs.h"
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

// Include Charm++ LB file
#include "vt/vrt/collection/balance/charmlb/greedy.h"
#include "vt/vrt/collection/balance/charmlb/kdlb.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

/*static*/ objgroup::proxy::Proxy<CharmLB> CharmLB::scatter_proxy = {};

void CharmLB::init(objgroup::proxy::Proxy<CharmLB> in_proxy) {
  proxy = scatter_proxy = in_proxy;
}

/*static*/ std::unordered_map<std::string, std::string>
CharmLB::getInputKeysWithHelp() {
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

void CharmLB::inputParams(balance::SpecEntry* spec) {
  auto keys_help = getInputKeysWithHelp();

  std::vector<std::string> allowed;
  for (auto&& elm : keys_help) {
    allowed.push_back(elm.first);
  }
  spec->checkAllowedKeys(allowed);
  min_threshold = spec->getOrDefault<double>("min", charm_threshold_p);
  max_threshold = spec->getOrDefault<double>("max", charm_max_threshold_p);
  auto_threshold = spec->getOrDefault<bool>("auto", charm_auto_threshold_p);

  balance::LBArgsEnumConverter<DataDistStrategy> strategy_converter_(
    "strategy", "DataDistStrategy", {
      {DataDistStrategy::scatter, "scatter"},
      {DataDistStrategy::pt2pt,   "pt2pt"},
      {DataDistStrategy::bcast,   "bcast"}
    }
  );
  strat_ = strategy_converter_.getFromSpec(spec, strat_);
}

void CharmLB::runLB(TimeType total_load) {
  this_load = loadMilli(total_load);
  buildHistogram();
  loadStats();
}

void CharmLB::loadStats() {
  auto const& this_node = theContext()->getNode();
  auto avg_load = getAvgLoad();
  auto total_load = getSumLoad();
  auto I = getStats()->at(lb::Statistic::P_l).at(lb::StatisticQuantity::imb);

  bool should_lb = false;
  this_load_begin = this_load;

  if (avg_load > 0.0000000001) {
    should_lb = I > charm_tolerance;
  }

  if (auto_threshold) {
    this_threshold = std::min(std::max(1.0f - I, min_threshold), max_threshold);
  }

  if (this_node == 0) {
    vt_print(
      lb,
      "CharmLB loadStats: load={}, total={}, avg={}, I={:.2f},"
      "should_lb={}, auto={}, threshold={}\n",
      TimeTypeWrapper(this_load / 1000), TimeTypeWrapper(total_load / 1000),
      TimeTypeWrapper(avg_load / 1000), I, should_lb, auto_threshold,
      TimeTypeWrapper(this_threshold / 1000)
    );
    fflush(stdout);
  }

  for (const auto obj : *load_model_) {
    if (obj.isMigratable()) {
      // Get LoadSummary object for this object containing all subphase loads
      // (note: 0 is an unused value)
      auto load = balance::getObjectLoads(
          load_model_, obj, {balance::PhaseOffset::NEXT_PHASE, 0});

      obj_loads.push_back(std::make_tuple(obj, load));
    }
  }

  if (should_lb) {
    calcLoadOver();
    reduceCollect();
  }
}

void CharmLB::collectHandler(CharmCollectMsg* msg) {
  vt_debug_print(
    normal, lb,
    "CharmLB::collectHandler: entries size={}\n",
    msg->getConstVal().getObjList().size()
  );

  auto objs = std::move(msg->getVal().getObjListMove());
  auto profile = std::move(msg->getVal().getLoadProfileMove());

  vtAssert(objs.size() > 0, "LB must have at least one object");
  const auto dimension = std::get<1>(objs[0]).subphase_loads.size();
  for (const auto& obj : objs) {
    vtAssert(std::get<1>(obj).subphase_loads.size() == dimension, "All objects must have equal number of subphases");
  }

  runBalancer<10>(dimension, std::move(objs),std::move(profile));
}

void CharmLB::reduceCollect() {
  vt_debug_print(
    verbose, lb,
    "CharmLB::reduceCollect: load={}, load_begin={} obj_loads.size()={}\n",
    TimeTypeWrapper(this_load / 1000),
    TimeTypeWrapper(this_load_begin / 1000), obj_loads.size()
  );
  using MsgType = CharmCollectMsg;
  auto cb = vt::theCB()->makeSend<CharmLB, MsgType, &CharmLB::collectHandler>(proxy[0]);
  auto msg = makeMessage<MsgType>(obj_loads,this_load);
  proxy.template reduce<collective::PlusOp<CharmPayload>>(msg.get(),cb);
}

template <int N>
void CharmLB::runBalancer(int dimension, ObjLoadListType &&in_objs,
                          LoadProfileType &&in_profile) {
  if (N == dimension) {
    runBalancerHelper<N>(std::move(in_objs), std::move(in_profile));
  } else {
    runBalancer<N - 1>(dimension, std::move(in_objs), std::move(in_profile));
  }
}

template <>
void CharmLB::runBalancer<1>(int dimension, ObjLoadListType &&in_objs,
                          LoadProfileType &&in_profile) {
    runBalancerHelper<1>(std::move(in_objs), std::move(in_profile));
}


template <int N>
void CharmLB::runBalancerHelper(
  ObjLoadListType&& in_objs, LoadProfileType&& in_profile
) {
  using ObjType = TreeStrategy::Obj<N>;
  using ProcType = TreeStrategy::Proc<N, false>;

  vt_print(lb, "CharmLB::runBalancer Running with {} dimensions\n", N);

  auto const& num_nodes = theContext()->getNumNodes();
  ObjLoadListType objs{std::move(in_objs)};
  LoadProfileType profile{std::move(in_profile)};
  std::vector<ObjType> recs;
  vt_debug_print(
    normal, lb,
    "CharmLB::runBalancer: objs={}, profile={}\n",
    objs.size(), profile.size()
  );

  for (auto&& obj : objs) {
    auto const& id = std::get<0>(obj);
    auto&& load = std::get<1>(obj);

    recs.emplace_back(id, load.whole_phase_load, std::move(load.subphase_loads));
  }

  auto nodes = std::vector<ProcType>{};
  for (NodeType n = 0; n < num_nodes; n++) {
    auto iter = profile.find(n);
    vtAssert(iter != profile.end(), "Must have load profile");
    std::array<LoadType, 1> speed = {0}; // Dummy for now
    nodes.emplace_back(n, iter->second, speed.data());
    vt_debug_print(
      verbose, lb,
      "\t CharmLB::runBalancer: node={}, profile={}\n",
      n, iter->second
    );
  }

  using Solution_t = TreeStrategy::Solution<ObjType, ProcType>;
  //auto strategy = TreeStrategy::Greedy<ObjType, ProcType, Solution_t>();
  auto strategy = TreeStrategy::RKdLB<ObjType, ProcType, Solution_t>();
  Solution_t solution(nodes);
  strategy.solve(recs, nodes, solution, false);

  std::vector<CharmDecision> decisions;
  return transferObjs(std::move(solution.decisions));
}

CharmLB::ObjIDType CharmLB::objSetNode(
  NodeType const& node, ObjIDType const& id
) {
  auto new_id = id;
  new_id.curr_node = node;
  return new_id;
}

void CharmLB::recvObjs(CharmSendMsg* msg) {
  vt_debug_print(
    normal, lb,
    "recvObjs: msg->transfer_.size={}\n", msg->transfer_.size()
  );
  recvObjsDirect(msg->transfer_.size(), msg->transfer_.data());
}

void CharmLB::recvObjsBcast(CharmBcastMsg* msg) {
  auto const n = theContext()->getNode();
  vt_debug_print(
    normal, lb,
    "recvObjs: msg->transfer_.size={}\n", msg->transfer_[n].size()
  );
  recvObjsDirect(msg->transfer_[n].size(), msg->transfer_[n].data());
}

void CharmLB::recvObjsDirect(std::size_t len, CharmLBTypes::ObjIDType* objs) {
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

/*static*/ void CharmLB::recvObjsHan(CharmLBTypes::ObjIDType* objs) {
  vt_debug_print(
    verbose, lb,
    "recvObjsHan: num_recs={}\n", *objs
  );
  scatter_proxy.get()->recvObjsDirect(static_cast<std::size_t>(objs->id), objs+1);
}

void CharmLB::transferObjs(std::vector<CharmDecision>&& in_decision) {
  std::size_t max_recs = 1;
  std::vector<CharmDecision> load(std::move(in_decision));
  std::vector<std::vector<CharmLBTypes::ObjIDType>> node_transfer(load.size());
  for (auto&& elm : load) {
    auto const& node = elm.node_;
    auto const& objs = elm.objs_;
    for (auto&& obj : objs) {
      auto const cur_node = objGetNode(obj);
      // transfer required from `cur_node' to `node'
      if (cur_node != node) {
        auto const new_obj_id = objSetNode(node, obj);
        node_transfer[cur_node].push_back(new_obj_id);
        max_recs = std::max(max_recs, node_transfer[cur_node].size() + 1);
      }
    }
  }

  if (strat_ == DataDistStrategy::scatter) {
    std::size_t max_bytes =  max_recs * sizeof(CharmLBTypes::ObjIDType);
    vt_debug_print(
      normal, lb,
      "CharmLB::transferObjs: max_recs={}, max_bytes={}\n",
      max_recs, max_bytes
    );
    theCollective()->scatter<CharmLBTypes::ObjIDType,recvObjsHan>(
      max_bytes*load.size(),max_bytes,nullptr,[&](NodeType node, void* ptr){
        auto ptr_out = reinterpret_cast<CharmLBTypes::ObjIDType*>(ptr);
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
      proxy[n].send<CharmSendMsg, &CharmLB::recvObjs>(node_transfer[n]);
    }
  } else if (strat_ == DataDistStrategy::bcast) {
    proxy.broadcast<CharmBcastMsg, &CharmLB::recvObjsBcast>(node_transfer);
  }
}

double CharmLB::getAvgLoad() const {
  return getStats()->at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);
}

double CharmLB::getMaxLoad() const {
  return getStats()->at(lb::Statistic::P_l).at(lb::StatisticQuantity::max);
}

double CharmLB::getSumLoad() const {
  return getStats()->at(lb::Statistic::P_l).at(lb::StatisticQuantity::sum);
}

void CharmLB::loadOverBin(ObjBinType bin, ObjBinListType& bin_list) {
  auto avg_load = getAvgLoad();
  auto const threshold = this_threshold * avg_load;
  auto const obj_id = bin_list.back();

  //load_over[bin].push_back(obj_id);
  bin_list.pop_back();

  auto const& obj_time_milli = loadMilli(load_model_->getWork(obj_id, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}));

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

void CharmLB::calcLoadOver() {
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

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_CHARMLB_CHARMLB_CC*/
