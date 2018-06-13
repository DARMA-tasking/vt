
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC

#include "config.h"
#include "vrt/collection/balance/greedylb/greedylb.h"
#include "vrt/collection/balance/greedylb/greedylb.fwd.h"
#include "vrt/collection/balance/greedylb/greedylb_types.h"
#include "vrt/collection/balance/greedylb/greedylb_constants.h"
#include "vrt/collection/balance/greedylb/greedylb_msgs.h"
#include "vrt/collection/balance/stats_msg.h"
#include "serialization/messaging/serialized_messenger.h"
#include "context/context.h"
#include "vrt/collection/manager.h"

#include <unordered_map>
#include <memory>
#include <vector>
#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace lb {

/*static*/ std::unique_ptr<GreedyLB> GreedyLB::greedy_lb_inst = nullptr;

/*static*/ void GreedyLB::greedyLBHandler(balance::GreedyLBMsg* msg) {
  auto const& phase = msg->getPhase();
  GreedyLB::greedy_lb_inst = std::make_unique<GreedyLB>();
  GreedyLB::greedy_lb_inst->start_time_ = timing::Timing::getCurrentTime();
  assert(balance::ProcStats::proc_data_.size() >= phase);
  GreedyLB::greedy_lb_inst->procDataIn(balance::ProcStats::proc_data_[phase]);
  greedy_lb_inst->reduceLoad();
}

void GreedyLB::reduceLoad() {
  debug_print(
    lblite, node,
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
    fmt::print(
      "VT: {}: "
      "loadStats: this_load={}, total_load={}, avg_load={}, max_load={}, "
      "diff={}, diff_percent={}, should_lb={}, auto={}, threshold={}\n",
      this_node, this_load, total_load, avg_load, max_load, diff, diff_percent,
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
    lblite, node,
    "CentralCollect: entries size={}\n",
    msg->getConstVal().getSample().size()
  );

  for (auto&& elm : msg->getConstVal().getSample()) {
    debug_print(
      lblite, node,
      "\t CentralCollect: bin={}, num={}\n",
      elm.first, elm.second.size()
    );
  }
}

void GreedyLB::reduceCollect() {
  #if greedylb_use_parserdes
    assert(0 && "greedylb parserdes not implemented");
  #else
    auto msg = makeSharedMessage<GreedyCollectMsg>(load_over);
    theCollective()->reduce<
      GreedyCollectMsg,
      GreedyCollectMsg::template msgHandler<
        GreedyCollectMsg, collective::PlusOp<GreedyPayload>, CentralCollect
      >
    >(greedy_root,msg);
  #endif
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
  assert(obj_iter != stats->end() && "Obj must exist in stats");
  auto const& obj_time_milli = loadMilli(obj_iter->second);

  this_load -= obj_time_milli;

  debug_print(
    lblite, node,
    "loadOverBin: this_load_begin={}, this_load={}, threshold={}: "
    "adding unit: bin={}, milli={}\n",
    this_load_begin, this_load, threshold, bin, obj_time_milli
  );
}

void GreedyLB::calcLoadOver() {
  auto const threshold = this_threshold * avg_load;

  debug_print(
    lblite, node,
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

  for (auto i = 0; i < obj_sample.size(); i++) {
    auto obj_iter = obj_sample.find(i);
    if (obj_iter != obj_sample.end() && obj_iter->second.size() == 0) {
      obj_sample.erase(obj_iter);
    }
  }
}

void GreedyLB::finishedLB() {
  auto const& this_node = theContext()->getNode();
  debug_print(
    lblite, node,
    "finished all transfers: count={}\n",
    transfer_count
  );
  if (this_node == 0) {
    auto const& total_time = timing::Timing::getCurrentTime() - start_time_;
    fmt::print(
      "VT: {}: "
      "loadStats: total_time={}, transfer_count={}\n",
      this_node, total_time, transfer_count
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
    lblite, node,
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
      lblite, node,
      "\t {}: procDataIn: this_load={}, obj={}, load={}, load_milli={}, bin={}\n",
      this_node, this_load, obj, load, load_milli, bin
    );
  }
  this_load_begin = this_load;
  stats = &data_in;
}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC*/
