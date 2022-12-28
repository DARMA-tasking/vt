/*
//@HEADER
// *****************************************************************************
//
//                                  lb_type.cc
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

#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/lb_type.h"

namespace vt { namespace vrt { namespace collection {

namespace balance {

static std::unordered_map<LBType,std::string> lb_names_ = {
  {LBType::NoLB,                std::string{"NoLB"               }},
# if vt_check_enabled(zoltan)
  {LBType::ZoltanLB,            std::string{"ZoltanLB"           }},
# endif
  {LBType::GreedyLB,            std::string{"GreedyLB"           }},
  {LBType::HierarchicalLB,      std::string{"HierarchicalLB"     }},
  {LBType::RotateLB,            std::string{"RotateLB"           }},
  {LBType::TemperedLB,          std::string{"TemperedLB"         }},
  {LBType::OfflineLB,           std::string{"OfflineLB"          }},
  {LBType::RandomLB,            std::string{"RandomLB"           }},
  {LBType::TestSerializationLB, std::string{"TestSerializationLB"}},
  {LBType::TemperedWMin,        std::string{"TemperedWMin"       }},
};

std::unordered_map<LBType, std::string>& get_lb_names() {
  return lb_names_;
}

} /* end namespace balance */

namespace lb {

static std::unordered_map<Statistic,std::string> lb_stat_names_ = {
  {Statistic::Rank_load_modeled,   std::string{"Rank_load_modeled"}},
  {Statistic::Rank_load_raw,       std::string{"Rank_load_raw"}},
  {Statistic::Rank_comm,           std::string{"Rank_comm"}},
  {Statistic::Rank_strategy_specific_load_modeled,
      std::string{"Rank_strategy_specific_load_modeled"}},
  {Statistic::Object_load_modeled, std::string{"Object_load_modeled"}},
  {Statistic::Object_load_raw,     std::string{"Object_load_raw"}},
  {Statistic::Object_comm,         std::string{"Object_comm"}},
  {Statistic::Object_strategy_specific_load_modeled,
      std::string{"Object_strategy_specific_load_modeled"}},
  {Statistic::ObjectRatio,         std::string{"ObjectRatio"}},
  {Statistic::EdgeRatio,           std::string{"EdgeRatio"}}
};

std::unordered_map<Statistic, std::string>& get_lb_stat_names() {
  return lb_stat_names_;
}

} /* end namespace lb */

}}} /* end namespace vt::vrt::collection::balance */
