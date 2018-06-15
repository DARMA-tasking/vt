
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_H

#include "config.h"
#include "vrt/collection/balance/greedylb/greedylb.fwd.h"
#include "vrt/collection/balance/greedylb/greedylb_types.h"
#include "vrt/collection/balance/greedylb/greedylb_constants.h"
#include "vrt/collection/balance/greedylb/greedylb_msgs.h"
#include "vrt/collection/balance/proc_stats.h"
#include "timing/timing.h"

#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include <list>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct GreedyLB : GreedyLBTypes {
  using ElementLoadType = std::unordered_map<ObjIDType,TimeType>;
  using ProcStatsMsgType = balance::ProcStatsMsg;
  using TransferType = std::map<NodeType, std::vector<ObjIDType>>;
  using LoadType = double;

  GreedyLB() = default;

public:
  void procDataIn(ElementLoadType const& data_in);

private:
  LoadType loadMilli(LoadType const& load);
  void reduceLoad();
  void loadStats(LoadType const& avg_load, LoadType const& max_load);
  static void loadStatsHandler(ProcStatsMsgType* msg);
  ObjBinType histogramSample(LoadType const& load);
  void finishedLB();
  void reduceCollect();
  void calcLoadOver();
  void loadOverBin(ObjBinType bin, ObjBinListType& bin_list);
  void runBalancer(ObjSampleType&& objs, LoadProfileType&& profile);
  void transferObjs(std::vector<GreedyProc>&& load);
  NodeType objGetNode(ObjIDType const& id);
  ObjIDType objSetNode(NodeType const& node, ObjIDType const& id);
  void recvObjsDirect(GreedyLBTypes::ObjIDType* objs);
  void finishedTransferExchange();
  static void recvObjsHan(GreedyLBTypes::ObjIDType* objs);

  struct GreedyAvgLoad {
    void operator()(balance::ProcStatsMsg* msg);
  };

  struct CentralCollect {
    void operator()(GreedyCollectMsg* msg);
  };

public:
  static void greedyLBHandler(balance::GreedyLBMsg* msg);
  static std::unique_ptr<GreedyLB> greedy_lb_inst;

private:
  double this_threshold = 0.0f;
  TimeType start_time_ = 0.0f;
  LoadType avg_load = 0.0f, max_load = 0.0f;
  LoadType this_load = 0.0f, this_load_begin = 0.0f;
  ElementLoadType const* stats = nullptr;
  ObjSampleType obj_sample, load_over;
  std::size_t load_over_size = 0;
  int64_t migrates_expected = 0, transfer_count = 0;
  TransferType transfers;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_H*/
