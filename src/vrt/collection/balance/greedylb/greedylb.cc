
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_CC

#include "config.h"
#include "vrt/collection/balance/greedylb/greedylb.h"
#include "vrt/collection/balance/greedylb/greedylb.fwd.h"
#include "vrt/collection/balance/greedylb/greedylb_constants.h"
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
