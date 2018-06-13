
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_TYPES_H
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_TYPES_H

#include "config.h"
#include "vrt/collection/balance/proc_stats.h"

#include <cstdlib>
#include <list>
#include <unordered_map>
#include <map>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct GreedyLBTypes {
  using ObjIDType = balance::ProcStats::ElementIDType;
  using ObjBinType = int32_t;
  using ObjBinListType = std::list<ObjIDType>;
  using ObjSampleType = std::map<ObjBinType, ObjBinListType>;
  using LoadType = double;
  using LoadProfileType = std::unordered_map<NodeType,LoadType>;
};

struct GreedyRecord {
  using ObjType = GreedyLBTypes::ObjIDType;
  using LoadType = GreedyLBTypes::LoadType;

  GreedyRecord(ObjType const& in_obj, LoadType const& in_load)
    : obj_(in_obj), load_(in_load)
  { }

  LoadType getLoad() const { return load_; }
  ObjType getObj() const { return obj_; }

private:
  GreedyLBTypes::ObjIDType obj_ = 0;
  LoadType load_ = 0.0f;
};

struct GreedyProc {
  GreedyProc() = default;
  GreedyProc(
    NodeType const& in_node, GreedyLBTypes::LoadType const& in_load
  ) : node_(in_node), load_(in_load) {}

  GreedyLBTypes::LoadType getLoad() const { return load_; }

  NodeType node_ = uninitialized_destination;
  GreedyLBTypes::LoadType load_ = 0.0f;
  std::vector<GreedyLBTypes::ObjIDType> recs_;
};

template <typename T>
struct GreedyCompareLoad {
  bool operator()(T const& p1, T const& p2) const {
    return p1.getLoad() < p2.getLoad();
  }
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_TYPES_H*/
