/*
//@HEADER
// *****************************************************************************
//
//                                  hierlb.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb.fwd.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_types.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_child.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_constants.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_msgs.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_strat.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/serialization/messaging/serialized_messenger.h"
#include "vt/collective/collective_alg.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/context/context.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/objgroup/headers.h"

#include <unordered_map>
#include <memory>
#include <list>
#include <vector>
#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace lb {

void HierarchicalLB::init(objgroup::proxy::Proxy<HierarchicalLB> in_proxy) {
  proxy = in_proxy;
}

/*static*/ std::unordered_map<std::string, std::string>
HierarchicalLB::getInputKeysWithHelp() {
  std::unordered_map<std::string, std::string> const keys_help = {
    {
      "min",
      R"(
Values: <double>
Default: 0.8
Description:
  The load threshold of objects to consider for potential migration on each
  rank. All objects over threshold * average_load on each rank will be
  considered. The order these will be selected is based on the value passed to
  "strategy". If the parameter "auto" is set to "true", this will be the minimum
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
Values: {LoadOverLessThan, LoadOverGreaterThan, LoadOverOneEach}
Default: LoadOverLessThan
Description:
  Select the strategy for which objects to select for rebalancing that are over
  the selected threshold:
   -  LoadOverLessThan    -- pick the smallest objects on the node
   -  LoadOverGreaterThan -- pick the largest objects on the node
   -  LoadOverOneEach     -- pick objects from all the sample bins across the
                             range of sizes
)"
    }
  };
  return keys_help;
}

void HierarchicalLB::inputParams(balance::ConfigEntry* config) {
  auto keys_help = getInputKeysWithHelp();

  std::vector<std::string> allowed;
  for (auto&& elm : keys_help) {
    allowed.push_back(elm.first);
  }
  config->checkAllowedKeys(allowed);
  min_threshold = config->getOrDefault<double>("min", hierlb_threshold_p);
  max_threshold = config->getOrDefault<double>("max", hierlb_max_threshold_p);
  auto_threshold = config->getOrDefault<bool>("auto", hierlb_auto_threshold_p);

  std::string extract = config->getOrDefault<std::string>(
    "strategy", "LoadOverLessThan"
  );
  if (extract.compare("LoadOverLessThan") == 0) {
    extract_strategy = HeapExtractEnum::LoadOverLessThan;
  } else if (extract.compare("LoadOverGreaterThan") == 0) {
    extract_strategy = HeapExtractEnum::LoadOverGreaterThan;
  } else if (extract.compare("LoadOverOneEach") == 0) {
    extract_strategy = HeapExtractEnum::LoadOverOneEach;
  } else {
    auto str =
      fmt::format("HierarchicalLB strategy={} is not valid", extract);
    vtAbort(str);
  }
}

void HierarchicalLB::setupTree(TimeTypeWrapper const threshold) {
  vtAssert(
    tree_setup == false,
    "Tree must not already be set up when is this called"
  );

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  this_threshold = threshold.seconds();

  vt_debug_print(
    terse, hierlb,
    "HierarchicalLB: setupTree: threshold={}\n",
    threshold
  );

  for (NodeType node = 0; node < hierlb_nary; node++) {
    NodeType const child = this_node * hierlb_nary + node + 1;
    if (child < num_nodes) {
      auto child_iter = children.find(child);
      vtAssert(child_iter == children.end(), "Child must not exist");
      children.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(child),
        std::forward_as_tuple(std::make_unique<HierLBChild>())
      );
      vt_debug_print(
        normal, hierlb,
        "\t{}: child={}\n", this_node, child
      );
    }
  }

  vt_debug_print(
    normal, hierlb,
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
        vtAssert(child_iter != children.end(), "Must exist");
        child_iter->second->final_child = true;
        vt_debug_print(
          normal, hierlb,
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

  vt_debug_print(
    normal, hierlb,
    "\t{}: parent={}, bottom_parent={}, children.size()={}\n",
    this_node, parent, bottom_parent, children.size()
  );
}

double HierarchicalLB::getAvgLoad() const {
  return getStats()->at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::avg
  );
}

double HierarchicalLB::getMaxLoad() const {
  return getStats()->at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::max
  );
}

double HierarchicalLB::getSumLoad() const {
  return getStats()->at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::sum
  );
}

void HierarchicalLB::loadStats() {
  auto const& this_node = theContext()->getNode();
  auto avg_load = getAvgLoad();
  auto total_load = getSumLoad();
  auto I = getStats()->at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::imb
  );

  bool should_lb = false;
  this_load_begin = this_load;

  if (avg_load > 0.0000000001) {
    should_lb = I > hierlb_tolerance;
  }

  if (auto_threshold) {
    this_threshold = std::min(std::max(1.0f - I, min_threshold), max_threshold);
  }

  if (this_node == 0) {
    vt_debug_print(
      terse, hierlb,
      "loadStats: load={}, total={}, avg={}, I={:.2f},"
      "should_lb={}, auto={}, threshold={}\n",
      TimeTypeWrapper(this_load / 1000), TimeTypeWrapper(total_load / 1000),
      TimeTypeWrapper(avg_load / 1000), I, should_lb, auto_threshold,
      TimeTypeWrapper(this_threshold / 1000)
    );
    if (!should_lb) {
      vt_print(
        lb,
        "HierarchicalLB decided to skip rebalancing due to low imbalance\n"
      );
    }
    fflush(stdout);
  }

  if (should_lb) {
    calcLoadOver(extract_strategy);

    lbTreeUpSend(
      bottom_parent, this_load, this_node, load_over, 1
    );

    if (children.size() == 0) {
      ObjSampleType empty_obj{};
      lbTreeUpSend(
        parent, hierlb_no_load_sentinel, this_node, empty_obj, agg_node_size
      );
    }
  }
}

void HierarchicalLB::loadOverBin(ObjBinType bin, ObjBinListType& bin_list) {
  auto const threshold = this_threshold * getAvgLoad();
  auto const obj_id = bin_list.back();

  load_over[bin].push_back(obj_id);
  bin_list.pop_back();

  auto const& obj_time_milli = loadMilli(load_model_->getModeledLoad(obj_id,
    {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}
  ));

  this_load -= obj_time_milli;

  vt_debug_print(
    normal, hierlb,
    "loadOverBin: this_load_begin={}, this_load={}, threshold={}: "
    "adding unit: bin={}, milli={}\n",
    TimeTypeWrapper(this_load_begin / 1000),
    TimeTypeWrapper(this_load / 1000), TimeTypeWrapper(threshold / 1000),
    bin, obj_time_milli
  );
}

void HierarchicalLB::calcLoadOver(HeapExtractEnum const extract) {
  auto const threshold = this_threshold * getAvgLoad();

  vt_debug_print(
    normal, hierlb,
    "calcLoadOver: this_load={}, avg_load={}, threshold={}, "
    "strategy={}\n",
    TimeTypeWrapper(this_load / 1000), TimeTypeWrapper(getAvgLoad() / 1000),
    TimeTypeWrapper(threshold / 1000),
    extract == HeapExtractEnum::LoadOverLessThan ? "LoadOverLessThan" :
    extract == HeapExtractEnum::LoadOverGreaterThan ? "LoadOverGreaterThan" :
    extract == HeapExtractEnum::LoadOverRandom ? "LoadOverRandom" :
    extract == HeapExtractEnum::LoadOverOneEach ? "LoadOverOneEach" : "?"
  );

  if (extract == HeapExtractEnum::LoadOverLessThan) {
    auto cur_item = obj_sample.begin();
    while (this_load > threshold && cur_item != obj_sample.end()) {
      if (cur_item->second.size() != 0) {
        loadOverBin(cur_item->first, cur_item->second);
      } else {
        cur_item++;
      }
    }
  } else if (extract == HeapExtractEnum::LoadOverGreaterThan) {
    auto cur_item = obj_sample.rbegin();
    while (this_load > threshold && cur_item != obj_sample.rend()) {
      if (cur_item->second.size() != 0) {
        loadOverBin(cur_item->first, cur_item->second);
      } else {
        cur_item++;
      }
    }
  }  else if (extract == HeapExtractEnum::LoadOverOneEach) {
    bool found = false;
    do {
      found = false;
      auto cur_item = obj_sample.begin();
      while (this_load > threshold && cur_item != obj_sample.end()) {
        if (cur_item->second.size() != 0) {
          loadOverBin(cur_item->first, cur_item->second);
          found = true;
        }
        cur_item++;
      }
    } while (found);
  }

  for (size_t i = 0; i < obj_sample.size(); i++) {
    auto obj_iter = obj_sample.find(i);
    if (obj_iter != obj_sample.end() && obj_iter->second.size() == 0) {
      obj_sample.erase(obj_iter);
    }
  }
}

void HierarchicalLB::downTreeHandler(LBTreeDownMsg* msg) {
  return downTree(msg->getFrom(), msg->getExcess(), msg->getFinalChild());
}

void HierarchicalLB::startMigrations() {
  vt_debug_print(
    normal, hierlb,
    "startMigrations\n"
  );

  auto const this_node = theContext()->getNode();

  for (auto&& bin : taken_objs) {
    for (auto&& obj_id : bin.second) {
      migrateObjectTo(obj_id, this_node);
    }
  }
}

void HierarchicalLB::downTreeSend(
  NodeType const node, NodeType const from, ObjSampleType const& excess,
  bool const final_child, std::size_t const& approx_size
) {
  proxy[node].template send<LBTreeDownMsg, &HierarchicalLB::downTreeHandler>(
    from, excess, final_child
  );
}

void HierarchicalLB::downTree(
  NodeType const from, ObjSampleType excess_load, bool const final_child
) {
  vt_debug_print(
    normal, hierlb,
    "downTree: from={}, bottom_parent={}: excess_load={}, final_child={}\n",
    from, bottom_parent, excess_load.size(), final_child
  );

  if (final_child) {
    // take the load
    taken_objs = excess_load;

    for (auto&& item : taken_objs) {
      LoadType const total_taken_load = item.first * item.second.size();

      vt_debug_print(
        verbose, hierlb,
        "downTree: from={}, taken_bin={}, taken_bin_count={}, "
        "total_taken_load={}\n",
        from, item.first, item.second.size(),
        TimeTypeWrapper(total_taken_load / 1000)
      );

      this_load += total_taken_load;
    }

    vt_debug_print(
      normal, hierlb,
      "downTree: this_load_begin={}, new load profile={}, avg_load={}\n",
      TimeTypeWrapper(this_load_begin / 1000),
      TimeTypeWrapper(this_load / 1000), TimeTypeWrapper(getAvgLoad() / 1000)
    );

    startMigrations();
  } else {
    given_objs = excess_load;
    sendDownTree();
  }
}

void HierarchicalLB::lbTreeUpHandler(LBTreeUpMsg* msg) {
  lbTreeUp(
    msg->getChildLoad(), msg->getChild(), msg->getModeledLoad(),
    msg->getChildSize()
  );
}

std::size_t HierarchicalLB::getSize(ObjSampleType const& sample) {
  return 0;
}

void HierarchicalLB::lbTreeUpSend(
  NodeType const node, LoadType const child_load, NodeType const child,
  ObjSampleType const& load, NodeType const child_size
) {
  proxy[node].template send<LBTreeUpMsg, &HierarchicalLB::lbTreeUpHandler>(
    child_load, child, load, child_size
  );
}

void HierarchicalLB::lbTreeUp(
  LoadType const child_load, NodeType const child, ObjSampleType load,
  NodeType const child_size
) {
  auto const& this_node = theContext()->getNode();

  vt_debug_print(
    normal, hierlb,
    "lbTreeUp: child={}, child_load={}, child_size={}, "
    "child_msgs={}, children.size()={}, agg_node_size={}, "
    "avg_load={}, child_avg={}, incoming load.size={}\n",
    child, TimeTypeWrapper(child_load / 1000), child_size, child_msgs+1,
    children.size(), agg_node_size + child_size,
    TimeTypeWrapper(getAvgLoad() / 1000),
    TimeTypeWrapper(child_load/child_size/1000),
    load.size()
  );

  LoadType total_child_load = 0.0f;
  if (load.size() > 0) {
    for (auto& bin : load) {
      vt_debug_print(
        verbose, hierlb,
        "\t lbTreeUp: combining bins for bin={}, size={}\n",
        bin.first, bin.second.size()
      );

      if (bin.second.size() > 0) {
        // splice in the new list to accumulated work units that fall in a
        // common histogram bin
        auto given_iter = given_objs.find(bin.first);

        if (given_iter == given_objs.end()) {
          // do the insertion here
          given_objs.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(bin.first),
            std::forward_as_tuple(ObjBinListType{})
          );

          given_iter = given_objs.find(bin.first);

          vtAssert(
            given_iter != given_objs.end(),
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

  vtAssert(
    child_iter != children.end(), "Entry must exist in children map"
  );

  child_iter->second->node_size = child_size;
  child_iter->second->cur_load = child_load;
  child_iter->second->node = child;

  total_child_load += child_load;

  child_msgs++;

  if (child_size > 0 && child_load != hierlb_no_load_sentinel) {
    auto live_iter = children.find(child);
    vtAssert(live_iter != children.end(), "Must exist");
    live_iter->second->is_live = true;
  }

  vtAssert(
    static_cast<size_t>(child_msgs) <= children.size(),
    "Number of children must be greater or less than"
  );

  if (static_cast<size_t>(child_msgs) == children.size()) {
    if (this_node == hierlb_root) {
      vt_debug_print(
        normal, hierlb,
        "lbTreeUp: reached root!: total_load={}, avg={}\n",
        TimeTypeWrapper(total_child_load / 1000),
        TimeTypeWrapper(total_child_load/agg_node_size/1000)
      );
      sendDownTree();
    } else {
      distributeAmongChildren();
    }
  }
}

HierLBChild* HierarchicalLB::findMinChild() {
  auto cur_iter = children.begin();
  while (!cur_iter->second->is_live && cur_iter != children.end()) {
    cur_iter++;
  }
  if (cur_iter == children.end()) {
    return nullptr;
  }

  auto cur = cur_iter->second.get();

  vt_debug_print(
    normal, hierlb,
    "findMinChild, cur->node={}, load={}\n",
    cur->node, TimeTypeWrapper(cur->cur_load / 1000)
  );

  for (auto&& c : children) {
    auto const& load = c.second->cur_load / c.second->node_size;
    auto const& cur_load = cur->cur_load / cur->node_size;
    vt_debug_print(
      verbose, hierlb,
      "\t findMinChild: CUR node={}, node_size={}, load={}, rel-load={}\n",
      cur->node, cur->node_size, TimeTypeWrapper(cur->cur_load / 1000),
      TimeTypeWrapper(cur_load / 1000)
    );
    vt_debug_print(
      verbose, hierlb,
      "\t findMinChild: C node={}, node_size={}, load={}, rel-load={}\n",
      c.second->node, c.second->node_size,
      TimeTypeWrapper(c.second->cur_load / 1000),
      TimeTypeWrapper(load / 1000)
    );
    if (load < cur_load && cur->is_live) {
      cur = c.second.get();
    }
  }

  return cur;
}

void HierarchicalLB::sendDownTree() {
  auto const& this_node = theContext()->getNode();

  vt_debug_print(
    normal, hierlb,
    "sendDownTree: given={}\n", given_objs.size()
  );

  auto cIter = given_objs.rbegin();

  while (cIter != given_objs.rend()) {
    auto c = findMinChild();
    int const weight = c->node_size;
    double const threshold = getAvgLoad() * weight * this_threshold;

    vt_debug_print(
      verbose, hierlb,
      "\t sendDownTree: distribute min child: c={}, child={}, cur_load={}, "
      "weight={}, avg_load={}, threshold={}\n",
      print_ptr(c), c ? c->node : -1, c ? c->cur_load : -1.0,
      weight, getAvgLoad(), threshold
    );

    if (c == nullptr || weight == 0) {
      break;
    } else {
      if (cIter->second.size() != 0) {
        vt_debug_print(
          verbose, hierlb,
          "\t sendDownTree: distribute: child={}, cur_load={}, time={}\n",
          c->node, TimeTypeWrapper(c->cur_load / 1000),
          TimeTypeWrapper(cIter->first)
       );

        // @todo agglomerate units into this bin together to increase efficiency
        auto found = c->recs.find(cIter->first) != c->recs.end();
        auto task = cIter->second.back();
        c->recs[cIter->first].push_back(task);
        c->cur_load += cIter->first;
        if (!found) {
          c->recs_size += sizeof(std::size_t) * 4;
          c->recs_size += sizeof(ObjBinType);
        }
        c->recs_size += sizeof(ObjIDType);

        // remove from list
        cIter->second.pop_back();
      } else {
        cIter++;
      }
    }
  }

  clearObj(given_objs);

  for (auto& c : children) {
    vt_debug_print(
      verbose, hierlb,
      "sendDownTree: downTreeSend: node={}, recs={}\n",
      c.second->node, c.second->recs.size()
    );

    for (auto&& elm : c.second->recs) {
      vt_debug_print(
        verbose, hierlb,
        "\t downTreeSend: node={}, recs={}, bin={}, bin_size={}, final={}\n",
        c.second->node, c.second->recs.size(), elm.first, elm.second.size(),
        c.second->final_child
      );
    }

    downTreeSend(
      c.second->node, this_node, c.second->recs, c.second->final_child,
      c.second->recs_size
    );
    c.second->recs_size = 0;
    c.second->recs.clear();
  }
}

void HierarchicalLB::distributeAmongChildren() {
  auto const& this_node = theContext()->getNode();

  vt_debug_print(
    normal, hierlb,
    "distributeAmongChildren: parent={}\n", parent
  );

  auto cIter = given_objs.rbegin();

  while (cIter != given_objs.rend()) {
    HierLBChild* c = findMinChild();
    int const weight = c->node_size;
    double const threshold = getAvgLoad() * weight * this_threshold;

    vt_debug_print(
      verbose, hierlb,
      "\t Up: distribute min child: c={}, child={}, cur_load={}, "
      "weight={}, avg_load={}, threshold={}\n",
      print_ptr(c),
      c ? c->node : -1,
      c ? c->cur_load : -1.0,
      weight, TimeTypeWrapper(getAvgLoad() / 1000),
      TimeTypeWrapper(threshold / 1000)
    );

    if (c == nullptr || c->cur_load > threshold || weight == 0) {
      break;
    } else {
      if (cIter->second.size() != 0) {
        vt_debug_print(
          verbose, hierlb,
          "\t Up: distribute: child={}, cur_load={}, time={}\n",
          c->node, TimeTypeWrapper(c->cur_load / 1000),
          TimeTypeWrapper(cIter->first)
        );

        // @todo agglomerate units into this bin together to increase efficiency
        auto found = c->recs.find(cIter->first) != c->recs.end();
        auto task = cIter->second.back();
        c->recs[cIter->first].push_back(task);
        c->cur_load += cIter->first;
        if (!found) {
          c->recs_size += sizeof(std::size_t) * 4;
          c->recs_size += sizeof(ObjBinType);
        }
        c->recs_size += sizeof(ObjIDType);

        // remove from list
        cIter->second.pop_back();
      } else {
        cIter++;
      }
    }
  }

  LoadType total_child_load = 0.0;
  NodeType total_size = 0;
  for (auto&& child : children) {
    auto const& node = child.second->node;
    auto const& node_size = child.second->node_size;
    auto const& load = child.second->cur_load;
    auto const& is_live = child.second->is_live;
    vt_debug_print(
      verbose, hierlb,
      "distributeAmongChildren: parent={}, child={}. is_live={}, size={}, "
      "load={}\n",
      parent, node, is_live, node_size, TimeTypeWrapper(load / 1000)
    );
    if (is_live) {
      total_child_load += load;
      total_size += node_size;
    }
  }

  clearObj(given_objs);
  lbTreeUpSend(
    parent, total_child_load, this_node, given_objs, total_size
  );

  given_objs.clear();
}

void HierarchicalLB::clearObj(ObjSampleType& objs) {
  std::vector<int> to_remove{};
  for (auto&& bin : objs) {
    if (bin.second.size() == 0) {
      to_remove.push_back(bin.first);
    }
  }
  for (auto&& r : to_remove) {
    auto giter = objs.find(r);
    vtAssert(giter != objs.end(), "Must exist");
    objs.erase(giter);
  }
}

void HierarchicalLB::runLB(TimeType total_load) {
  this_load = loadMilli(total_load);
  buildHistogram();
  setupTree(min_threshold);

  proxy.allreduce<&HierarchicalLB::setupDone>();
}

void HierarchicalLB::setupDone() {
  loadStats();
}

}}}} /* end namespace vt::vrt::collection::lb */
