/*
//@HEADER
// *****************************************************************************
//
//                                  lb_type.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H
#define INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H

#include "vt/config.h"

#include <unordered_map>
#include <string>
#include <type_traits>

namespace vt { namespace vrt { namespace collection { namespace balance {

enum struct LBType : int8_t {
    NoLB             = 0
  , GreedyLB         = 1
  , HierarchicalLB   = 2
  , RotateLB         = 3
  , TemperedLB       = 4
  , StatsMapLB       = 5
# if vt_check_enabled(zoltan)
  , ZoltanLB         = 6
# endif
  , RandomLB         = 7
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

extern std::unordered_map<LBType,std::string> lb_names_;

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_LB_TYPE_H*/
