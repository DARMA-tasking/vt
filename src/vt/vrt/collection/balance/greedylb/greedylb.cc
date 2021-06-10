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
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/serialization/messaging/serialized_messenger.h"
#include "vt/context/context.h"
#include "vt/vrt/collection/manager.h"
#include "vt/collective/reduce/reduce.h"

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

void GreedyLB::inputParams(balance::SpecEntry* spec) {
  std::vector<std::string> allowed{"min", "max", "auto"};
  spec->checkAllowedKeys(allowed);
  min_threshold = spec->getOrDefault<double>("min", greedy_threshold_p);
  max_threshold = spec->getOrDefault<double>("max", greedy_max_threshold_p);
  auto_threshold = spec->getOrDefault<bool>("auto", greedy_auto_threshold_p);
}

void GreedyLB::runLB() {
  loadStats();
}

void GreedyLB::loadStats() {
  auto const& this_node = theContext()->getNode();
  auto avg_load = getAvgLoad();
  auto total_load = getSumLoad();
  auto I = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::imb);

  bool should_lb = false;
  this_load_begin = this_load;

  if (avg_load > 0.0000000001) {
    should_lb = I > greedy_tolerance;
  }

  if (auto_threshold) {
    this_threshold = std::min(std::max(1.0f - I, min_threshold), max_threshold);
  }

  if (this_node == 0) {
    vt_print(
      lb,
      "loadStats: load={:.2f}, total={:.2f}, avg={:.2f}, I={:.2f},"
      "should_lb={}, auto={}, threshold={}\n",
      this_load, total_load, avg_load, I, should_lb, auto_threshold,
      this_threshold
    );
    fflush(stdout);
  }

  if (should_lb) {
    calcLoadOver();
    reduceCollect();
  }
}

void GreedyLB::collectHandler(GreedyCollectMsg* msg) {
  vt_debug_print(
    normal, lb,
    "GreedyLB::collectHandler: entries size={}\n",
    msg->getConstVal().getSample().size()
  );

  for (auto&& elm : msg->getConstVal().getSample()) {
    vt_debug_print(
      verbose, lb,
      "\t collectHandler: bin={}, num={}\n",
      elm.first, elm.second.size()
    );
  }

  auto objs = std::move(msg->getVal().getSampleMove());
  auto profile = std::move(msg->getVal().getLoadProfileMove());
  runBalancer(std::move(objs),std::move(profile));
}

void GreedyLB::reduceCollect() {
  vt_debug_print(
    verbose, lb,
    "GreedyLB::reduceCollect: load={}, load_begin={} load_over.size()={}\n",
    this_load, this_load_begin, load_over.size()
  );
  using MsgType = GreedyCollectMsg;
  auto cb = vt::theCB()->makeSend<GreedyLB, MsgType, &GreedyLB::collectHandler>(proxy[0]);
  auto msg = makeMessage<MsgType>(load_over,this_load);
  proxy.template reduce<collective::PlusOp<GreedyPayload>>(msg.get(),cb);
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
      min_node.node_, min_node.load_, min_node.recs_.size(),
      max_rec.getObj(), max_rec.getLoad()
    );
    min_node.recs_.push_back(max_rec.getObj());
    min_node.load_ += max_rec.getLoad();
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
  recvObjsDirect(msg->transfer_.size(), &msg->transfer_[0]);
}

void GreedyLB::recvObjsBcast(GreedyBcastMsg* msg) {
  auto const n = theContext()->getNode();
  vt_debug_print(
    normal, lb,
    "recvObjs: msg->transfer_.size={}\n", msg->transfer_[n].size()
  );
  recvObjsDirect(msg->transfer_[n].size(), &msg->transfer_[n][0]);
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
#define SCATTER 1
#define PT2PT 0
#define BROADCAST 0

#if SCATTER
  std::size_t max_recs = 1, max_bytes = 0;
#endif
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
#if SCATTER
        max_recs = std::max(max_recs, node_transfer[cur_node].size() + 1);
#endif
      }
    }
  }

#if SCATTER
  max_bytes =  max_recs * sizeof(GreedyLBTypes::ObjIDType);
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
#elif PT2PT
  for (NodeType n = 0; n < theContext()->getNumNodes(); n++) {
    vtAssert(
      node_transfer.size() == static_cast<size_t>(theContext()->getNumNodes()),
      "Must contain all nodes"
    );
    proxy[n].send<GreedySendMsg, &GreedyLB::recvObjs>(node_transfer[n]);
  }
#elif BROADCAST
  proxy.broadcast<GreedyBcastMsg, &GreedyLB::recvObjsBcast>(node_transfer);
#else
# error "Must select some strategy for distribuing info"
#endif

#undef BROADCAST
#undef PT2PT
#undef SCATTER
}

double GreedyLB::getAvgLoad() const {
  return stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);
}

double GreedyLB::getMaxLoad() const {
  return stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::max);
}

double GreedyLB::getSumLoad() const {
  return stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::sum);
}

void GreedyLB::loadOverBin(ObjBinType bin, ObjBinListType& bin_list) {
  auto avg_load = getAvgLoad();
  auto const threshold = this_threshold * avg_load;
  auto const obj_id = bin_list.back();

  load_over[bin].push_back(obj_id);
  bin_list.pop_back();

  auto const& obj_time_milli = loadMilli(load_model_->getWork(obj_id, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}));

  this_load -= obj_time_milli;

  vt_debug_print(
    normal, lb,
    "loadOverBin: this_load_begin={}, this_load={}, threshold={}: "
    "adding unit: bin={}, milli={}\n",
    this_load_begin, this_load, threshold, bin, obj_time_milli
  );
}

void GreedyLB::calcLoadOver() {
  auto avg_load = getAvgLoad();
  auto const threshold = this_threshold * avg_load;

  vt_debug_print(
    normal, lb,
    "calcLoadOver: this_load={}, avg_load={}, threshold={}\n",
    this_load, avg_load, threshold
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
