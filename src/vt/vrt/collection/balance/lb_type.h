/*
//@HEADER
// ************************************************************************
//
//                          lb_type.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H
#define INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H

#include "vt/config.h"

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

template <typename SerializerT>
void serialize(SerializerT& s, LBType lb) {
  using EnumDataType = typename std::underlying_type<LBType>::type;
  EnumDataType val = static_cast<EnumDataType>(lb);
  s | val;
  lb = static_cast<LBType>(val);
}

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
