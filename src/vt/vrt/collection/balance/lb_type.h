
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H
#define INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H

#include "vt/config.h"

#include <unordered_map>
#include <string>

namespace vt { namespace vrt { namespace collection { namespace balance {

enum struct LBType : int8_t {
  NoLB             = 0,
  GreedyLB         = 1,
  HierarchicalLB   = 2,
  RotateLB         = 3
};

template <typename=void> std::unordered_map<LBType,std::string> lb_names_ = {
  {LBType::NoLB,           std::string{"NoLB"          }},
  {LBType::GreedyLB,       std::string{"GreedyLB"      }},
  {LBType::HierarchicalLB, std::string{"HierarchicalLB"}},
  {LBType::RotateLB,       std::string{"RotateLB"      }}
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H*/
