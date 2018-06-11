
#include "config.h"
#include "vrt/collection/balance/hierarchicallb/hierlb.h"
#include "vrt/collection/balance/hierarchicallb/hierlb.fwd.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_types.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_child.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_constants.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_msgs.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_strat.h"
#include "vrt/collection/balance/stats_msg.h"
#include "serialization/messaging/serialized_messenger.h"
#include "collective/collective_alg.h"
#include "collective/reduce/reduce.h"
#include "context/context.h"
#include "vrt/collection/manager.h"

#include <unordered_map>
#include <memory>
#include <list>
#include <vector>
#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace lb {

/*static*/
std::unique_ptr<HierarchicalLB> HierarchicalLB::hier_lb_inst = nullptr;

void HierarchicalLB::setupTree(double const threshold) {
  assert(
    tree_setup == false &&
    "Tree must not already be set up when is this called"
  );

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  this_threshold = threshold;

  debug_print(
    hierlb, node,
    "HierarchicalLB: setupTree: threshold={}\n",
    threshold
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

HierarchicalLB::ObjBinType HierarchicalLB::histogramSample(
  LoadType const& load
) {
  ObjBinType const bin =
    ((static_cast<int32_t>(load)) / hierlb_bin_size * hierlb_bin_size)
    + hierlb_bin_size;
  return bin;
}

HierarchicalLB::LoadType HierarchicalLB::loadMilli(LoadType const& load) {
  return load * 1000;
}

void HierarchicalLB::procDataIn(ElementLoadType const& data_in) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    hierlb, node,
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
      hierlb, node,
      "\t {}: procDataIn: this_load={}, obj={}, load={}, load_milli={}, bin={}\n",
      this_node, this_load, obj, load, load_milli, bin
    );
  }
  this_load_begin = this_load;
  stats = &data_in;
}

void HierarchicalLB::HierAvgLoad::operator()(balance::ProcStatsMsg* msg) {
  auto nmsg = makeSharedMessage<balance::ProcStatsMsg>(*msg);
  theMsg()->broadcastMsg<
    balance::ProcStatsMsg, HierarchicalLB::loadStatsHandler
  >(nmsg);
  auto nmsg_root = makeSharedMessage<balance::ProcStatsMsg>(*msg);
  HierarchicalLB::loadStatsHandler(nmsg_root);
}

/*static*/ void HierarchicalLB::loadStatsHandler(ProcStatsMsgType* msg) {
  auto const& lmax = msg->getConstVal().loadMax();
  auto const& lsum = msg->getConstVal().loadSum();
  HierarchicalLB::hier_lb_inst->loadStats(lsum,lmax);
}

void HierarchicalLB::reduceLoad() {
  debug_print(
    hierlb, node,
    "reduceLoad: this_load={}\n", this_load
  );
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
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  avg_load = total_load / num_nodes;
  max_load = in_max_load;

  auto const& diff = max_load - avg_load;
  auto const& diff_percent = (diff / total_load) * 100.0f;
  bool const& should_lb = diff_percent > hierlb_tolerance;

  if (should_lb && hierlb_auto_threshold) {
    this_threshold = std::min(
      std::max(1.0f - (diff_percent / 100.0f), hierlb_threshold),
      hierlb_max_threshold
    );
  }

  if (this_node == 0) {
    fmt::print(
      "VT: {}: "
      "loadStats: this_load={}, total_load={}, avg_load={}, max_load={}, "
      "diff={}, diff_percent={}, should_lb={}, auto={}, threshold={}\n",
      this_node, this_load, total_load, avg_load, max_load, diff, diff_percent,
      should_lb, hierlb_auto_threshold, this_threshold
    );
  }

  if (should_lb) {
    calcLoadOver(HeapExtractEnum::LoadOverLessThan);

    lbTreeUpSend(
      bottom_parent, this_load, this_node, load_over, 1, load_over_size
    );

    if (children.size() == 0) {
      auto const& total_size = sizeof(std::size_t) * 4;
      ObjSampleType empty_obj{};
      lbTreeUpSend(
        parent, hierlb_no_load_sentinel, this_node, empty_obj, agg_node_size,
        total_size
      );
    }
  } else {
    // release continuation for next iteration
    finishedTransferExchange();
  }
}

void HierarchicalLB::loadOverBin(ObjBinType bin, ObjBinListType& bin_list) {
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
    hierlb, node,
    "loadOverBin: this_load_begin={}, this_load={}, threshold={}: "
    "adding unit: bin={}, milli={}\n",
    this_load_begin, this_load, threshold, bin, obj_time_milli
  );
}

void HierarchicalLB::calcLoadOver(HeapExtractEnum const extract) {
  auto const threshold = this_threshold * avg_load;

  debug_print(
    hierlb, node,
    "calcLoadOver: this_load={}, avg_load={}, threshold={}\n",
    this_load, avg_load, threshold
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

  for (auto i = 0; i < obj_sample.size(); i++) {
    auto obj_iter = obj_sample.find(i);
    if (obj_iter != obj_sample.end() && obj_iter->second.size() == 0) {
      obj_sample.erase(obj_iter);
    }
  }
}

/*static*/ void HierarchicalLB::downTreeHandler(LBTreeDownMsg* msg) {
  HierarchicalLB::hier_lb_inst->downTree(
    msg->getFrom(), msg->getExcess(), msg->getFinalChild()
  );
}

NodeType HierarchicalLB::objGetNode(ObjIDType const& id) {
  return id & 0x0000000FFFFFFFF;
}

void HierarchicalLB::finishedTransferExchange() {
  debug_print(
    hierlb, node,
    "finished all transfers: count={}\n",
    transfer_count
  );
  balance::ProcStats::proc_migrate_.clear();
  balance::ProcStats::proc_data_.clear();
  balance::ProcStats::next_elm_ = 1;
  theCollection()->releaseLBContinuation();
}

void HierarchicalLB::startMigrations() {
  auto const& this_node = theContext()->getNode();
  TransferType transfer_list;

  EpochType const epoch = theTerm()->newEpoch();
  theMsg()->setGlobalEpoch(epoch);
  theTerm()->attachEpochTermAction(epoch,[this]{
    this->finishedTransferExchange();
  });

  for (auto&& bin : taken_objs) {
    for (auto&& obj_id : bin.second) {
      auto const& node = objGetNode(obj_id);

      if (node != this_node) {
        migrates_expected++;

        debug_print(
          hierlb, node,
          "startMigrations, obj_id={}, node={}\n",
          obj_id, node
        );

        transfer_list[node].push_back(obj_id);
      }
    }
  }

  debug_print(
    hierlb, node,
    "startMigrations, transfer_list.size()={}\n",
    transfer_list.size()
  );

  for (auto&& trans : transfer_list) {
    transferSend(trans.first, this_node, trans.second);
  }
}

void HierarchicalLB::transferSend(
  NodeType node, NodeType from, std::vector<ObjIDType> transfer
) {
  #if hierlb_use_parserdes
    auto const& size =
      transfer.size() * sizeof(ObjIDType) + (sizeof(std::size_t) * 2);
    auto msg = makeSharedMessageSz<TransferMsg>(size,from,transfer);
    SerializedMessenger::sendParserdesMsg<TransferMsg,transferHan>(node,msg);
  #else
    auto msg = makeSharedMessage<TransferMsg>(from,transfer);
    SerializedMessenger::sendSerialMsg<TransferMsg,transferHan>(node,msg);
  #endif
}

void HierarchicalLB::transfer(NodeType from, std::vector<ObjIDType> list) {
  auto trans_iter = transfers.find(from);

  assert(trans_iter == transfers.end() && "There must not be an entry");

  transfers[from] = list;
  transfer_count += list.size();

  for (auto&& elm : list) {
    debug_print(
      hierlb, node,
      "transfer: list.size()={}, elm={}\n",
      list.size(), elm
    );

    auto iter = balance::ProcStats::proc_migrate_.find(elm);
    assert(iter != balance::ProcStats::proc_migrate_.end() && "Must exist");
    iter->second(from);
  }
}

/*static*/ void HierarchicalLB::transferHan(TransferMsg* msg) {
  HierarchicalLB::hier_lb_inst->transfer(msg->getFrom(), msg->getTransfer());
}

void HierarchicalLB::downTreeSend(
  NodeType const node, NodeType const from, ObjSampleType const& excess,
  bool const final_child, std::size_t const& approx_size
) {
  #if hierlb_use_parserdes
    auto msg = makeSharedMessageSz<LBTreeDownMsg>(
      approx_size,from,excess,final_child
    );
    SerializedMessenger::sendParserdesMsg<LBTreeDownMsg,downTreeHandler>(
      node,msg
    );
  #else
    auto msg = makeSharedMessage<LBTreeDownMsg>(from,excess,final_child);
    SerializedMessenger::sendSerialMsg<LBTreeDownMsg,downTreeHandler>(node,msg);
  #endif
}

void HierarchicalLB::downTree(
  NodeType const from, ObjSampleType excess_load, bool const final_child
) {
  debug_print(
    hierlb, node,
    "downTree: from={}, bottom_parent={}: excess_load={}, final_child={}\n",
    from, bottom_parent, excess_load.size(), final_child
  );

  if (final_child) {
    // take the load
    taken_objs = std::move(excess_load);

    for (auto&& item : taken_objs) {
      LoadType const total_taken_load = item.first * item.second.size();

      debug_print(
        hierlb, node,
        "downTree: from={}, taken_bin={}, taken_bin_count={}, "
        "total_taken_load={}\n",
        from, item.first, item.second.size(), total_taken_load
      );

      this_load += total_taken_load;
    }

    debug_print(
      hierlb, node,
      "downTree: this_load_begin={}, new load profile={}, avg_load={}\n",
      this_load_begin, this_load, avg_load
    );

    startMigrations();
  } else {
    given_objs = std::move(excess_load);
    sendDownTree();
  }
}

/*static*/ void HierarchicalLB::lbTreeUpHandler(LBTreeUpMsg* msg) {
  HierarchicalLB::hier_lb_inst->lbTreeUp(
    msg->getChildLoad(), msg->getChild(), msg->getLoad(),
    msg->getChildSize()
  );
}

std::size_t HierarchicalLB::getSize(ObjSampleType const& sample) {
  return 0;
}

void HierarchicalLB::lbTreeUpSend(
  NodeType const node, LoadType const child_load, NodeType const child,
  ObjSampleType const& load, NodeType const child_size,
  std::size_t const& load_size_approx
) {
  #if hierlb_use_parserdes
    auto msg = makeSharedMessageSz<LBTreeUpMsg>(
      load_size_approx,child_load,child,load,child_size
    );
    SerializedMessenger::sendParserdesMsg<LBTreeUpMsg,lbTreeUpHandler>(node,msg);
  #else
    auto msg = makeSharedMessage<LBTreeUpMsg>(child_load,child,load,child_size);
    SerializedMessenger::sendSerialMsg<LBTreeUpMsg,lbTreeUpHandler>(node,msg);
  #endif
}

void HierarchicalLB::lbTreeUp(
  LoadType const child_load, NodeType const child, ObjSampleType load,
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

  LoadType total_child_load = 0.0f;
  if (load.size() > 0) {
    for (auto& bin : load) {
      debug_print(
        hierlb, node,
        "\t lbTreeUp: combining bins for bin={}, size={}\n",
        bin.first, bin.second.size()
      );

      if (bin.second.size() > 0) {
        // splice in the new list to accumulated work units that fall in a
        // common histrogram bin
        auto given_iter = given_objs.find(bin.first);

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

  if (child_size > 0 && child_load != hierlb_no_load_sentinel) {
    auto live_iter = children.find(child);
    assert(live_iter != children.end() && "Must exist");
    live_iter->second->is_live = true;
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

HierLBChild* HierarchicalLB::findMinChild() {
  auto cur_iter = children.begin();
  while (!cur_iter->second->is_live && cur_iter != children.end()) {
    cur_iter++;
  }
  if (cur_iter == children.end()) {
    return nullptr;
  }

  auto cur = cur_iter->second.get();

  debug_print(
    hierlb, node,
    "findMinChild, cur->node={}, load={}\n",
    cur->node, cur->cur_load
  );

  for (auto&& c : children) {
    auto const& load = c.second->cur_load / c.second->node_size;
    auto const& cur_load = cur->cur_load / cur->node_size;
    debug_print(
      hierlb, node,
      "\t findMinChild: CUR node={}, node_size={}, load={}, rel-load={}\n",
      cur->node, cur->node_size, cur->cur_load, cur_load
    );
    debug_print(
      hierlb, node,
      "\t findMinChild: C node={}, node_size={}, load={}, rel-load={}\n",
      c.second->node, c.second->node_size, c.second->cur_load, load
    );
    if (load < cur_load && cur->is_live) {
      cur = c.second.get();
    }
  }

  return cur;
}

void HierarchicalLB::sendDownTree() {
  auto const& this_node = theContext()->getNode();

  debug_print(
    hierlb, node,
    "sendDownTree: given={}\n", given_objs.size()
  );

  auto cIter = given_objs.rbegin();

  while (cIter != given_objs.rend()) {
    auto c = findMinChild();
    int const weight = c->node_size;
    double const threshold = avg_load * weight * this_threshold;

    debug_print(
      hierlb, node,
      "\t sendDownTree: distribute min child: c={}, child={}, cur_load={}, "
      "weight={}, avg_load={}, threshold={}\n",
      print_ptr(c), c ? c->node : -1, c ? c->cur_load : -1.0,
      weight, avg_load, threshold
    );

    if (c == nullptr || weight == 0) {
      break;
    } else {
      if (cIter->second.size() != 0) {
        debug_print(
          hierlb, node,
          "\t sendDownTree: distribute: child={}, cur_load={}, time={}\n",
          c->node, c->cur_load, cIter->first
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
    debug_print(
      hierlb, node,
      "sendDownTree: downTreeSend: node={}, recs={}\n",
      c.second->node, c.second->recs.size()
    );

    for (auto&& elm : c.second->recs) {
      debug_print(
        hierlb, node,
        "\t downTreeSend: node={}, recs={}, bin={}, bin_size={}\n",
        c.second->node, c.second->recs.size(), elm.first, elm.second.size()
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

void HierarchicalLB::distributeAmoungChildren() {
  auto const& this_node = theContext()->getNode();

  debug_print(
    hierlb, node,
    "distributeAmoungChildren: parent={}\n", parent
  );

  auto cIter = given_objs.rbegin();

  while (cIter != given_objs.rend()) {
    HierLBChild* c = findMinChild();
    int const weight = c->node_size;
    double const threshold = avg_load * weight * this_threshold;

    debug_print(
      hierlb, node,
      "\t Up: distribute min child: c={}, child={}, cur_load={}, "
      "weight={}, avg_load={}, threshold={}\n",
      print_ptr(c),
      c ? c->node : -1,
      c ? c->cur_load : -1.0,
      weight, avg_load, threshold
    );

    if (c == nullptr || c->cur_load > threshold || weight == 0) {
      break;
    } else {
      if (cIter->second.size() != 0) {
        debug_print(
          hierlb, node,
          "\t Up: distribute: child={}, cur_load={}, time={}\n",
          c->node, c->cur_load, cIter->first
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
    debug_print(
      hierlb, node,
      "distributeAmoungChildren: parent={}, child={}. is_live={}, size={}, "
      "load={}\n",
      parent, node, is_live, node_size, load
    );
    if (is_live) {
      total_child_load += load;
      total_size += node_size;
    }
  }

  auto const& data_size = clearObj(given_objs);
  lbTreeUpSend(
    parent, total_child_load, this_node, given_objs, total_size, data_size
  );

  given_objs.clear();
}

std::size_t HierarchicalLB::clearObj(ObjSampleType& objs) {
  std::size_t total_size = 0;
  std::vector<int> to_remove{};
  for (auto&& bin : objs) {
    if (bin.second.size() == 0) {
      to_remove.push_back(bin.first);
    }
    total_size += bin.second.size() * sizeof(ObjIDType);
    total_size += sizeof(ObjBinType);
    total_size += sizeof(std::size_t) * 4;
  }
  for (auto&& r : to_remove) {
    auto giter = objs.find(r);
    assert(giter != objs.end() && "Must exist");
    objs.erase(giter);
  }
  return total_size;
}

/*static*/ void HierarchicalLB::hierLBHandler(balance::HierLBMsg* msg) {
  auto const& phase = msg->getPhase();
  HierarchicalLB::hier_lb_inst = std::make_unique<HierarchicalLB>();
  HierarchicalLB::hier_lb_inst->setupTree(hierlb_threshold);
  assert(balance::ProcStats::proc_data_.size() >= phase);
  HierarchicalLB::hier_lb_inst->procDataIn(balance::ProcStats::proc_data_[phase]);
  HierarchicalLB::hier_lb_inst->reduceLoad();
}

}}}} /* end namespace vt::vrt::collection::lb */
