
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H

#include "config.h"
#include "vrt/collection/balance/hierarchicallb/hierlb.fwd.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_types.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_child.h"
#include "vrt/collection/balance/proc_stats.h"

#include <unordered_map>
#include <memory>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct HierarchicalLB : HierLBTypes {
  using ChildPtrType = std::unique_ptr<HierLBChild>;
  using ChildMapType = std::unordered_map<NodeType,ChildPtrType>;
  using ElementLoadType = std::unordered_map<ObjIDType,TimeType>;
  using ProcStatsMsgType = balance::ProcStatsMsg;
  using LoadType = double;

  HierarchicalLB() = default;

  void setupTree();
  void calcLoadOver();
  void procDataIn(ElementLoadType const& data_in);

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
  static std::unique_ptr<HierarchicalLB> hier_lb_inst;

private:
  bool tree_setup = false;
  NodeType parent = uninitialized_destination;
  NodeType bottom_parent = uninitialized_destination;
  ChildMapType children, live_children;
  LoadType avg_load = 0.0f, max_load = 0.0f;
  LoadType this_load = 0.0f;
  ObjSampleType obj_sample;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H*/
