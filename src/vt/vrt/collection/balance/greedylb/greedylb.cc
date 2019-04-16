/*
//@HEADER
// ************************************************************************
//
//                          greedylb.cc
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC

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

#include <unordered_map>
#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace lb {

/*static*/ std::unique_ptr<GreedyLB> GreedyLB::greedy_lb_inst = nullptr;

/*static*/ void GreedyLB::greedyLBHandler(balance::GreedyLBMsg* msg) {
  auto const& phase = msg->getPhase();
  GreedyLB::greedy_lb_inst = std::make_unique<GreedyLB>();

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
        GreedyLB::greedy_lb_inst->this_threshold = spec->min();
        GreedyLB::greedy_lb_inst->greedy_threshold = spec->min();
        has_min_only = true;
      }
      if (spec->hasMax()) {
        GreedyLB::greedy_lb_inst->this_threshold = spec->max();
        GreedyLB::greedy_lb_inst->greedy_max_threshold = spec->min();
        has_min_only = false;
      }
      if (has_min_only) {
        GreedyLB::greedy_lb_inst->greedy_auto_threshold = false;
      }
      fallback = false;
    }
  }

  if (fallback) {
    GreedyLB::greedy_lb_inst->greedy_auto_threshold = greedy_auto_threshold_p;
    GreedyLB::greedy_lb_inst->greedy_threshold = greedy_threshold_p;
    GreedyLB::greedy_lb_inst->greedy_max_threshold = greedy_max_threshold_p;
    GreedyLB::greedy_lb_inst->this_threshold = greedy_threshold_p;
  }

  GreedyLB::greedy_lb_inst->start_time_ = timing::Timing::getCurrentTime();
  vtAssertExpr(balance::ProcStats::proc_data_.size() >= phase);
  GreedyLB::greedy_lb_inst->procDataIn(balance::ProcStats::proc_data_[phase]);
  greedy_lb_inst->reduceLoad();
}

void GreedyLB::reduceLoad() {
  debug_print(
    lb, node,
    "reduceLoad: this_load={}\n", this_load
  );
  auto msg = makeSharedMessage<ProcStatsMsgType>(this_load);
  theCollective()->reduce<
    ProcStatsMsgType,
    ProcStatsMsgType::template msgHandler<
      ProcStatsMsgType, collective::PlusOp<balance::LoadData>, GreedyAvgLoad
    >
  >(greedy_root,msg);
}

GreedyLB::LoadType GreedyLB::loadMilli(LoadType const& load) {
  return load * 1000;
}

GreedyLB::ObjBinType GreedyLB::histogramSample(LoadType const& load) {
  ObjBinType const bin =
    ((static_cast<int32_t>(load)) / greedy_bin_size * greedy_bin_size)
    + greedy_bin_size;
  return bin;
}

void GreedyLB::GreedyAvgLoad::operator()(balance::ProcStatsMsg* msg) {
  auto nmsg = makeSharedMessage<balance::ProcStatsMsg>(*msg);
  theMsg()->broadcastMsg<
    balance::ProcStatsMsg, GreedyLB::loadStatsHandler
  >(nmsg);
  auto nmsg_root = makeSharedMessage<balance::ProcStatsMsg>(*msg);
  GreedyLB::loadStatsHandler(nmsg_root);
}

/*static*/ void GreedyLB::loadStatsHandler(ProcStatsMsgType* msg) {
  auto const& lmax = msg->getConstVal().loadMax();
  auto const& lsum = msg->getConstVal().loadSum();
  GreedyLB::greedy_lb_inst->loadStats(lsum,lmax);
}

void GreedyLB::loadStats(
  LoadType const& total_load, LoadType const& in_max_load
) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  avg_load = total_load / num_nodes;
  max_load = in_max_load;

  auto const& diff = max_load - avg_load;
  auto const& diff_percent = (diff / avg_load) * 100.0f;
  bool const& should_lb = diff_percent > greedy_tolerance;

  if (should_lb && greedy_auto_threshold) {
    this_threshold = std::min(
      std::max(1.0f - (diff_percent / 100.0f), greedy_threshold),
      greedy_max_threshold
    );
  }

  if (this_node == 0) {
    vt_print(
      lblite,
      "loadStats: this_load={}, total_load={}, avg_load={}, max_load={}, "
      "diff={}, diff_percent={}, should_lb={}, auto={}, threshold={}\n",
      this_load, total_load, avg_load, max_load, diff, diff_percent,
      should_lb, greedy_auto_threshold, this_threshold
    );
    fflush(stdout);
  }

  if (should_lb) {
    calcLoadOver();
    reduceCollect();
  } else {
    // release continuation for next iteration
    finishedLB();
  }
}

void GreedyLB::CentralCollect::operator()(GreedyCollectMsg* msg) {
  debug_print(
    lb, node,
    "CentralCollect: entries size={}\n",
    msg->getConstVal().getSample().size()
  );

  for (auto&& elm : msg->getConstVal().getSample()) {
    debug_print(
      lb, node,
      "\t CentralCollect: bin={}, num={}\n",
      elm.first, elm.second.size()
    );
  }

  auto objs = std::move(msg->getVal().getSampleMove());
  auto profile = std::move(msg->getVal().getLoadProfileMove());
  greedy_lb_inst->runBalancer(std::move(objs),std::move(profile));
}

void GreedyLB::reduceCollect() {
  #if greedylb_use_parserdes
    vtAssert(0, "greedylb parserdes not implemented");
  #else
    debug_print(
      lb, node,
      "GreedyLB::reduceCollect: load={}, load_begin={} load_over.size()={}\n",
      this_load, this_load_begin, load_over.size()
    );
    auto msg = makeSharedMessage<GreedyCollectMsg>(load_over,this_load);
    theCollective()->reduce<
      GreedyCollectMsg,
      GreedyCollectMsg::template msgHandler<
        GreedyCollectMsg, collective::PlusOp<GreedyPayload>, CentralCollect
      >
    >(greedy_root,msg);
  #endif
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
  debug_print(
    lb, node,
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
    debug_print(
      lb, node,
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
    debug_print(
      lb, node,
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

NodeType GreedyLB::objGetNode(ObjIDType const& id) {
  return id & 0x0000000FFFFFFFF;
}

GreedyLB::ObjIDType GreedyLB::objSetNode(
  NodeType const& node, ObjIDType const& id
) {
  auto const new_id = id & 0xFFFFFFFF0000000;
  return new_id | node;
}

void GreedyLB::finishedTransferExchange() {
  auto const& this_node = theContext()->getNode();
  debug_print(
    lb, node,
    "finished all transfers: count={}\n",
    transfer_count
  );
  if (this_node == 0) {
    auto const& total_time = timing::Timing::getCurrentTime() - start_time_;
    vt_print(
      lblite,
      "loadStats: total_time={}, transfer_count={}\n",
      total_time, transfer_count
    );
    fflush(stdout);
  }
  balance::ProcStats::proc_migrate_.clear();
  balance::ProcStats::proc_data_.clear();
  balance::ProcStats::next_elm_ = 1;
  theCollection()->releaseLBContinuation();
}

void GreedyLB::recvObjsDirect(GreedyLBTypes::ObjIDType* objs) {
  auto const& this_node = theContext()->getNode();
  auto const& num_recs = *objs;
  auto recs = objs + 1;
  debug_print(
    lb, node,
    "recvObjsDirect: num_recs={}\n", num_recs
  );
  TransferType transfer_list;
  auto const epoch = theTerm()->newEpoch();
  theMsg()->pushEpoch(epoch);
  theTerm()->addActionEpoch(epoch,[this]{
    this->finishedTransferExchange();
  });
  for (decltype(+num_recs) i = 0; i < num_recs; i++) {
    auto const& to_node = objGetNode(recs[i]);
    auto const& new_obj_id = objSetNode(this_node,recs[i]);
    debug_print(
      lb, node,
      "\t recvObjs: i={}, to_node={}, obj={}, new_obj_id={}, num_recs={}, "
      "byte_offset={}\n",
      i, to_node, recs[i], new_obj_id, num_recs,
      reinterpret_cast<char*>(recs) - reinterpret_cast<char*>(objs)
    );
    auto iter = balance::ProcStats::proc_migrate_.find(new_obj_id);
    vtAssert(iter != balance::ProcStats::proc_migrate_.end(), "Must exist");
    iter->second(to_node);
    transfer_count++;
  }
  theTerm()->finishedEpoch(epoch);
  theMsg()->popEpoch();
}

/*static*/ void GreedyLB::recvObjsHan(GreedyLBTypes::ObjIDType* objs) {
  GreedyLB::greedy_lb_inst->recvObjsDirect(objs);
}

void GreedyLB::transferObjs(std::vector<GreedyProc>&& in_load) {
  std::size_t max_recs = 0, max_bytes = 0;
  std::vector<GreedyProc> load(std::move(in_load));
  std::vector<std::vector<GreedyLBTypes::ObjIDType>> node_transfer(load.size());
  for (auto&& elm : load) {
    auto const& node = elm.node_;
    auto const& recs = elm.recs_;
    for (auto&& rec : recs) {
      auto const& cur_node = objGetNode(rec);
      // transfer required from `cur_node' to `node'
      if (cur_node != node) {
        auto const new_obj_id = objSetNode(node, rec);
        node_transfer[cur_node].push_back(new_obj_id);
        max_recs = std::max(max_recs, node_transfer[cur_node].size() + 1);
      }
    }
  }
  max_bytes =  max_recs * sizeof(GreedyLBTypes::ObjIDType);
  debug_print(
    lb, node,
    "GreedyLB::transferObjs: max_recs={}, max_bytes={}\n",
    max_recs, max_bytes
  );
  theCollective()->scatter<GreedyLBTypes::ObjIDType,recvObjsHan>(
    max_bytes*load.size(),max_bytes,nullptr,[&](NodeType node, void* ptr){
      auto ptr_out = reinterpret_cast<GreedyLBTypes::ObjIDType*>(ptr);
      auto const& proc = node_transfer[node];
      auto const& rec_size = proc.size();
      *ptr_out = rec_size;
      for (size_t i = 0; i < rec_size; i++) {
        *(ptr_out + i + 1) = proc[i];
      }
    }
  );
}

void GreedyLB::loadOverBin(ObjBinType bin, ObjBinListType& bin_list) {
  auto const threshold = this_threshold * avg_load;
  auto const obj_id = bin_list.back();

  if (load_over.find(bin) == load_over.end()) {
    load_over_size += sizeof(std::size_t) * 4;
    load_over_size += sizeof(ObjBinType);
  }
  load_over_size += sizeof(ObjIDType);

  load_over[bin].push_back(obj_id);
  bin_list.pop_back();

  auto obj_iter = stats->find(obj_id);
  vtAssert(obj_iter != stats->end(), "Obj must exist in stats");
  auto const& obj_time_milli = loadMilli(obj_iter->second);

  this_load -= obj_time_milli;

  debug_print(
    lb, node,
    "loadOverBin: this_load_begin={}, this_load={}, threshold={}: "
    "adding unit: bin={}, milli={}\n",
    this_load_begin, this_load, threshold, bin, obj_time_milli
  );
}

void GreedyLB::calcLoadOver() {
  auto const threshold = this_threshold * avg_load;

  debug_print(
    lb, node,
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

void GreedyLB::finishedLB() {
  auto const& this_node = theContext()->getNode();
  debug_print(
    lb, node,
    "finished all transfers: count={}\n",
    transfer_count
  );
  if (this_node == 0) {
    auto const& total_time = timing::Timing::getCurrentTime() - start_time_;
    vt_print(
      lblite,
      "GreedyLB: loadStats: total_time={}, transfer_count={}\n",
      total_time, transfer_count
    );
    fflush(stdout);
  }
  balance::ProcStats::proc_migrate_.clear();
  balance::ProcStats::proc_data_.clear();
  balance::ProcStats::next_elm_ = 1;
  theCollection()->releaseLBContinuation();
}

void GreedyLB::procDataIn(ElementLoadType const& data_in) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    lb, node,
    "{}: procDataIn: size={}\n", this_node, data_in.size()
  );
  for (auto&& stat : data_in) {
    auto const& obj = stat.first;
    auto const& load = stat.second;
    auto const& load_milli = loadMilli(load);
    auto const& bin = histogramSample(load_milli);
    this_load += load_milli;
    obj_sample[bin].push_back(obj);

    debug_print(
      lb, node,
      "\t {}: procDataIn: this_load={}, obj={}, load={}, load_milli={}, bin={}\n",
      this_node, this_load, obj, load, load_milli, bin
    );
  }
  this_load_begin = this_load;
  stats = &data_in;
}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC*/
