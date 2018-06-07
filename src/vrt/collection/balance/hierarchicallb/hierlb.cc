
#include "config.h"
#include "vrt/collection/balance/hierarchicallb/hierlb.h"
#include "vrt/collection/balance/hierarchicallb/hierlb.fwd.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_types.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_child.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_constants.h"
#include "vrt/collection/balance/stats_msg.h"
#include "collective/collective_alg.h"
#include "collective/reduce/reduce.h"
#include "context/context.h"

#include <unordered_map>
#include <memory>
#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace lb {

/*static*/
std::unique_ptr<HierarchicalLB> HierarchicalLB::hier_lb_inst =
  std::make_unique<HierarchicalLB>();

void HierarchicalLB::setupTree() {
  assert(
    tree_setup == false &&
    "Tree must not already be set up when is this called"
  );

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  debug_print(
    hierlb, node,
    "HierarchicalLB: setupTree\n"
  );

  for (NodeType node = 0; node < hierlb_nary; node++) {
    NodeType const child = this_node * hierlb_nary + node + 1;
    if (child < num_nodes) {
      auto child_iter = children.find(child);
      assert(child_iter == children.end() && "Child must not exist");
      children.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(child),
        std::forward_as_tuple(std::make_unique<HierLBChild>())
      );
      debug_print(
        hierlb, node,
        "\t{}: child={}\n", this_node, child
      );
    }
  }

  debug_print(
    hierlb, node,
    "HierarchicalLB: num children={}\n",
    children.size()
  );

  if (children.size() == 0) {
    for (NodeType node = 0; node < hierlb_nary; node++) {
      NodeType factor = num_nodes / hierlb_nary * hierlb_nary;
      if (factor < num_nodes) {
        factor += hierlb_nary;
      }
      NodeType const child = (this_node * hierlb_nary + node + 1) - factor - 1;
      if (child < num_nodes && child >= 0) {
        children.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(child),
          std::forward_as_tuple(std::make_unique<HierLBChild>())
        );
        auto child_iter = children.find(child);
        assert(child_iter != children.end() && "Must exist");
        child_iter->second->final_child = true;
        debug_print(
          hierlb, node,
          "\t{}: child-x={}\n", this_node, child
        );
      }
    }
  }

  parent = (this_node - 1) / hierlb_nary;

  NodeType factor = num_nodes / hierlb_nary * hierlb_nary;
  if (factor < num_nodes) {
    factor += hierlb_nary;
  }

  bottom_parent = ((this_node + 1 + factor) - 1) / hierlb_nary;

  debug_print(
    hierlb, node,
    "\t{}: parent={}, bottom_parent={}, children.size()={}\n",
    this_node, parent, bottom_parent, children.size()
  );
}

HierarchicalLB::ObjBinType
HierarchicalLB::histogramSample(LoadType const& load) {
  ObjBinType const bin =
    ((static_cast<int32_t>(load)) / hierlb_bin_size * hierlb_bin_size)
    + hierlb_bin_size;
  return bin;
}

HierarchicalLB::LoadType
HierarchicalLB::loadMilli(LoadType const& load) {
  return load * 1000;
}

void HierarchicalLB::procDataIn(ElementLoadType const& data_in) {
  for (auto&& stat : data_in) {
    auto const& obj = stat.first;
    auto const& load = stat.second;
    auto const& load_milli = loadMilli(load);
    auto const& bin = histogramSample(load_milli);
    this_load += load_milli;
    obj_sample[bin].push_back(obj);
  }
}

void HierarchicalLB::HierAvgLoad::operator()(balance::ProcStatsMsg* msg) {
  auto nmsg = makeSharedMessage<balance::ProcStatsMsg>(*msg);
  theMsg()->broadcastMsg<
    balance::ProcStatsMsg, HierarchicalLB::loadStatsHandler
  >(nmsg);
}

/*static*/ void HierarchicalLB::loadStatsHandler(ProcStatsMsgType* msg) {
  auto const& lmax = msg->getConstVal().loadMax();
  auto const& lsum = msg->getConstVal().loadSum();
  HierarchicalLB::hier_lb_inst->loadStats(lsum,lmax);
}

void HierarchicalLB::reduceLoad() {
  auto msg = makeSharedMessage<ProcStatsMsgType>(this_load);
  theCollective()->reduce<
    ProcStatsMsgType,
    ProcStatsMsgType::template msgHandler<
      ProcStatsMsgType, collective::PlusOp<balance::LoadData>, HierAvgLoad
    >
  >(hierlb_root,msg);
}

void HierarchicalLB::loadStats(
  LoadType const& total_load, LoadType const& in_max_load
) {
  auto const& num_nodes = theContext()->getNumNodes();
  avg_load = total_load / num_nodes;
  max_load = in_max_load;

  debug_print(
    hierlb, node,
    "loadStats: total_load={}, avg_load={}, max_load={}\n",
    total_load, avg_load, max_load
  );
}

void HierarchicalLB::calcLoadOver() {
}

}}}} /* end namespace vt::vrt::collection::lb */
