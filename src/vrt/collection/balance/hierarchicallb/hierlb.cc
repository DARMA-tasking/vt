
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
#include <list>
#include <vector>
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
  stats = &data_in;
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

  calcLoadOver();
}

void HierarchicalLB::calcLoadOver() {
  auto const threshold = hierlb_threshold * avg_load;

  debug_print(
    hierlb, node,
    "calcLoadOver: this_load={}, avg_load={}, threshold={}\n",
    this_load, avg_load, threshold
  );

  auto cur_item = obj_sample.begin();
  while (this_load > threshold && cur_item != obj_sample.end()) {
    if (cur_item->second.size() != 0) {
      auto const obj = cur_item->second.back();

      load_over[cur_item->first].push_back(obj);
      cur_item->second.pop_back();

      auto const& obj_id = obj;
      auto obj_iter = stats->find(obj_id);
      assert(obj_iter != stats->end() && "Obj must exist in stats");
      auto const& obj_time_milli = loadMilli(obj_iter->second);

      this_load -= obj_time_milli;

      debug_print(
        hierlb, node,
        "calcLoadOver: this_load={}, threshold={}, adding unit: bin={}\n",
        this_load, threshold, cur_item->first
      );
    } else {
      cur_item++;
    }
  }

  for (auto i = 0; i < obj_sample.size(); i++) {
    if (obj_sample[i].size() == 0) {
      obj_sample.erase(obj_sample.find(i));
    }
  }
}

void HierarchicalLB::lbTreeUp(
  LoadType const child_load, NodeType const child, ObjSampleType&& load,
  NodeType const child_size
) {
  auto const& this_node = theContext()->getNode();

  debug_print(
    hierlb, node,
    "lbTreeUp: child={}, child_load={}, child_size={}, "
    "child_msgs={}, children.size()={}, agg_node_size={}, "
    "avg_load={}, child_avg={}, incoming load.size={}\n",
    child, child_load, child_size, child_msgs+1, children.size(),
    agg_node_size + child_size, avg_load, child_load/child_size,
    load.size()
  );

  if (load.size() > 0) {
    for (auto& bin : load) {
      debug_print(
        hierlb, node,
        "\t lbTreeUp: combining bins for bin={}, size=%{}\n",
        bin.first, bin.second.size()
      );

      if (bin.second.size() > 0) {
        // splice in the new list to accumulated work units that fall in a
        // common histrogram bin
        typename decltype(given_objs)::iterator given_iter =
          given_objs.find(bin.first);

        if (given_iter == given_objs.end()) {
          // do the insertion here
          given_objs.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(bin.first),
            std::forward_as_tuple(ObjBinListType{})
          );

          given_iter = given_objs.find(bin.first);

          assert(
            given_iter != given_objs.end() &&
            "An insertion just took place so this must not fail"
          );
        }

        // add in the load that was just received
        total_child_load += bin.first * bin.second.size();

        given_iter->second.splice(given_iter->second.begin(), bin.second);
      }
    }
  }

  agg_node_size += child_size;

  auto child_iter = children.find(child);

  assert(
    child_iter != children.end() && "Entry must exist in children map"
  );

  child_iter->second->node_size = child_size;
  child_iter->second->cur_load = child_load;
  child_iter->second->node = child;

  total_child_load += child_load;

  child_msgs++;

  if (child_size > 0 && child_load != 0.0) {
    auto live_iter = live_children.find(child);
    if (live_iter == live_children.end()) {
      live_children.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(child),
        std::forward_as_tuple(
          std::make_unique<HierLBChild>(*child_iter->second)
        )
      );
    }
  }

  assert(
    child_msgs <= children.size() &&
    "Number of children must be greater or less than"
  );

  if (child_msgs == children.size()) {
    if (this_node == hierlb_root) {
      debug_print(
        hierlb, node,
        "lbTreeUp: reached root!: total_load={}, avg={}\n",
        total_child_load, total_child_load/agg_node_size
      );
      sendDownTree();
    } else {
      distributeAmoungChildren();
    }
  }
}

}}}} /* end namespace vt::vrt::collection::lb */
