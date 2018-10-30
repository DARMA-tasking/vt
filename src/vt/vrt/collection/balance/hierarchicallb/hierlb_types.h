
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_TYPES_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_TYPES_H

#include "config.h"
#include "vrt/collection/balance/proc_stats.h"

#include <cstdlib>
#include <list>
#include <unordered_map>
#include <map>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct HierLBTypes {
  using ObjIDType = balance::ProcStats::ElementIDType;
  using ObjBinType = int32_t;
  using ObjBinListType = std::list<ObjIDType>;
  using ObjSampleType = std::map<ObjBinType, ObjBinListType>;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_TYPES_H*/
