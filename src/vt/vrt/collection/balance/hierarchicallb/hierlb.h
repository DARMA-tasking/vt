
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb.fwd.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_types.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_child.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_msgs.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_strat.h"
#include "vt/vrt/collection/balance/proc_stats.h"
#include "vt/timing/timing.h"

#include <unordered_map>
#include <vector>
#include <list>
#include <map>
#include <memory>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct HierarchicalLB : HierLBTypes {
  using ChildPtrType = std::unique_ptr<HierLBChild>;
  using ChildMapType = std::unordered_map<NodeType,ChildPtrType>;
  using ElementLoadType = std::unordered_map<ObjIDType,TimeType>;
  using ProcStatsMsgType = balance::ProcStatsMsg;
  using TransferType = std::map<NodeType, std::vector<ObjIDType>>;
  using LoadType = double;

  HierarchicalLB() = default;

  void setupTree(double const threshold);
  void calcLoadOver(HeapExtractEnum const extract);
  void loadOverBin(ObjBinType bin, ObjBinListType& bin_list);
  void procDataIn(ElementLoadType const& data_in);

private:
  static void downTreeHandler(LBTreeDownMsg* msg);
  static void transferHan(TransferMsg* msg);
  static void lbTreeUpHandler(LBTreeUpMsg* msg);

  void downTreeSend(
    NodeType const node, NodeType const from, ObjSampleType const& excess,
    bool const final_child, std::size_t const& approx_size
  );
  void lbTreeUpSend(
    NodeType const node, LoadType const child_load, NodeType const child,
    ObjSampleType const& load, NodeType const child_size,
    std::size_t const& load_size_approx
  );
  void transferSend(NodeType to, NodeType from, std::vector<ObjIDType> list);

  void downTree(
    NodeType const from, ObjSampleType&& excess, bool const final_child
  );
  void lbTreeUp(
    LoadType const child_load, NodeType const child, ObjSampleType&& load,
    NodeType const child_size
  );
  void transfer(NodeType from, std::vector<ObjIDType>&& list);

  void sendDownTree();
  void distributeAmoungChildren();
  std::size_t clearObj(ObjSampleType& objs);
  HierLBChild* findMinChild();
  void startMigrations();
  NodeType objGetNode(ObjIDType const& id);
  void finishedTransferExchange();

private:
  ObjBinType histogramSample(LoadType const& load);
  LoadType loadMilli(LoadType const& load);
  std::size_t getSize(ObjSampleType const&);

private:
  void reduceLoad();
  void loadStats(LoadType const& avg_load, LoadType const& max_load);
  static void loadStatsHandler(ProcStatsMsgType* msg);

  struct HierAvgLoad {
    void operator()(balance::ProcStatsMsg* msg);
  };

public:
  static void hierLBHandler(balance::HierLBMsg* msg);
  static std::unique_ptr<HierarchicalLB> hier_lb_inst;

private:
  double hierlb_max_threshold = 0.0f, hierlb_threshold = 0.0f;
  bool hierlb_auto_threshold = true;
  TimeType start_time_ = 0.0f;
  double this_threshold = 0.0f;
  bool tree_setup = false;
  NodeType parent = uninitialized_destination;
  NodeType bottom_parent = uninitialized_destination;
  NodeType agg_node_size = 0, child_msgs = 0;
  ChildMapType children;
  LoadType avg_load = 0.0f, max_load = 0.0f;
  LoadType this_load = 0.0f, this_load_begin = 0.0f;
  ObjSampleType obj_sample, load_over, given_objs, taken_objs;
  std::size_t load_over_size = 0;
  ElementLoadType const* stats = nullptr;
  int64_t migrates_expected = 0, transfer_count = 0;
  TransferType transfers;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H*/
