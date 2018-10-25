
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H
#define INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H

#include "config.h"

#include <unordered_map>
#include <string>
#include <type_traits>

namespace vt { namespace vrt { namespace collection { namespace balance {

enum struct LBType : int8_t {
  NoLB             = 0,
  GreedyLB         = 1,
  HierarchicalLB   = 2,
  RotateLB         = 3
};

}}}} /* end namespace vt::vrt::collection::balance */

namespace std {

using LBTypeType = vt::vrt::collection::balance::LBType;

template <>
struct hash<LBTypeType> {
  size_t operator()(LBTypeType const& in) const {
    using LBUnderType = typename std::underlying_type<LBTypeType>::type;
    auto const val = static_cast<LBUnderType>(in);
    return std::hash<LBUnderType>()(val);
  }
};

} /* end namespace std */

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename=void> std::unordered_map<LBType,std::string> lb_names_ = {
  {LBType::NoLB,           std::string{"NoLB"          }},
  {LBType::GreedyLB,       std::string{"GreedyLB"      }},
  {LBType::HierarchicalLB, std::string{"HierarchicalLB"}},
  {LBType::RotateLB,       std::string{"RotateLB"      }}
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H*/
