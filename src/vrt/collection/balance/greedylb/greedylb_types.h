
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
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_TYPES_H*/
