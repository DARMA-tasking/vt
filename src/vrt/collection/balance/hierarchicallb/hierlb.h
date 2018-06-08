
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H

#include "config.h"
#include "vrt/collection/balance/hierarchicallb/hierlb.fwd.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_types.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_child.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_msgs.h"
#include "vrt/collection/balance/proc_stats.h"

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

  void setupTree();
  void calcLoadOver();
  void procDataIn(ElementLoadType const& data_in);

private:
  static void downTreeHandler(LBTreeDownMsg* msg);
  static void transferHan(TransferMsg* msg);
  static void lbTreeUpHandler(LBTreeUpMsg* msg);

  void downTreeSend(
    NodeType const node, NodeType const from, ObjSampleType const& excess,
    bool const final_child
  );
  void lbTreeUpSend(
    NodeType const node, LoadType const child_load, NodeType const child,
    ObjSampleType const& load, NodeType const child_size
  );
  void transferSend(NodeType to, NodeType from, std::vector<ObjIDType> list);

  void downTree(
    NodeType const from, ObjSampleType excess, bool const final_child
  );
  void lbTreeUp(
    LoadType const child_load, NodeType const child, ObjSampleType load,
    NodeType const child_size
  );
  void transfer(NodeType from, std::vector<ObjIDType> list);

  void sendDownTree();
  void distributeAmoungChildren();
  void clearObj(ObjSampleType& objs);
  HierLBChild* findMinChild();
  void startMigrations();
  NodeType objGetNode(ObjIDType const& id);
  void finishedTransferExchange();

private:
  ObjBinType histogramSample(LoadType const& load);
  LoadType loadMilli(LoadType const& load);

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
  bool tree_setup = false;
  NodeType parent = uninitialized_destination;
  NodeType bottom_parent = uninitialized_destination;
  NodeType agg_node_size = 0, child_msgs = 0;
  ChildMapType children, live_children;
  LoadType avg_load = 0.0f, max_load = 0.0f, total_child_load = 0.0f;
  LoadType this_load = 0.0f;
  ObjSampleType obj_sample, load_over, given_objs, taken_objs;
  ElementLoadType const* stats = nullptr;
  int64_t migrates_expected = 0, transfer_count = 0;
  TransferType transfers;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H*/
