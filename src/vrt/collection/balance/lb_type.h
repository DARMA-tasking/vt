
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H
#define INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H

#include "config.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

enum struct LBType : int8_t {
  NoLB = 0,
  GreedyLB = 1,
  HierarchicalLB = 2
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H*/
