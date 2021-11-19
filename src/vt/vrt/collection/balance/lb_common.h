/*
//@HEADER
// *****************************************************************************
//
//                                 lb_common.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_COMMON_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_COMMON_H

#include "vt/config.h"
#include "vt/timing/timing_type.h"
#include "vt/messaging/message/message.h"

#include <cstdlib>
#include <unordered_map>
#include <ostream>

#include <fmt/ostream.h>

namespace vt { namespace vrt { namespace collection {
namespace balance {

using ElementIDType = uint64_t;

struct ElementIDStruct {
  using isByteCopyable = std::true_type;

  ElementIDStruct() = default;
  ElementIDStruct(
    ElementIDType in_id, NodeType in_home_node, NodeType in_curr_node
  ) : id(in_id),
      home_node(in_home_node),
      curr_node(in_curr_node)
  { }

  // id must be unique across nodes
  ElementIDType id = 0;
  NodeType home_node = uninitialized_destination;
  NodeType curr_node = uninitialized_destination;

  bool operator==(const ElementIDStruct& rhs) const {
    return id == rhs.id;
  }

  bool operator<(const ElementIDStruct& rhs) const {
    return id < rhs.id;
  }
};

std::ostream& operator<<(
  std::ostream& os, const ::vt::vrt::collection::balance::ElementIDStruct& id
);

static constexpr ElementIDType const no_element_id = 0;

using LoadMapType         = std::unordered_map<ElementIDStruct,TimeType>;
using SubphaseLoadMapType = std::unordered_map<ElementIDStruct, std::vector<TimeType>>;
} /* end namespace balance */

namespace lb {

enum struct StatisticQuantity : int8_t {
  min, max, avg, std, var, skw, kur, car, imb, npr, sum
};

enum struct Statistic : int8_t {
  P_l, P_c, P_t,
  O_l, O_c, O_t,
  // W_l_min, W_l_max, W_l_avg, W_l_std, W_l_var, W_l_skewness, W_l_kurtosis,
  // W_c_min, W_c_max, W_c_avg, W_c_std, W_c_var, W_c_skewness, W_c_kurtosis,
  // W_t_min, W_t_max, W_t_avg, W_t_std, W_t_var, W_t_skewness, W_t_kurtosis,
  // ObjectCardinality,
  ObjectRatio,
  // EdgeCardinality,
  EdgeRatio,
  // ExternalEdgesCardinality,
  // InternalEdgesCardinality
};

} /* end namespace lb */

}}} /* end namespace vt::vrt::collection */

namespace std {

using StatisticType = vt::vrt::collection::lb::Statistic;

template <>
struct hash<StatisticType> {
  size_t operator()(StatisticType const& in) const {
    using StatisticUnderType = typename std::underlying_type<StatisticType>::type;
    auto const val = static_cast<StatisticUnderType>(in);
    return std::hash<StatisticUnderType>()(val);
  }
};

} /* end namespace std */

namespace vt { namespace vrt { namespace collection { namespace lb {

std::unordered_map<Statistic, std::string>& get_lb_stat_name();

}}}} /* end namespace vt::vrt::collection::lb */

namespace std {

using ElementIDStructType = vt::vrt::collection::balance::ElementIDStruct;
using ElementIDMemberType = vt::vrt::collection::balance::ElementIDType;

template <>
struct hash<ElementIDStructType> {
  size_t operator()(ElementIDStructType const& in) const {
    return std::hash<ElementIDMemberType>()(in.id);
  }
};

} /* end namespace std */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_COMMON_H*/
