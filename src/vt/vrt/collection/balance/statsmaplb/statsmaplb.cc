/*
//@HEADER
// ************************************************************************
//
//                          StatsMapLB.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/statsmaplb/statsmaplb.h"
#include "vt/vrt/collection/balance/stats_lb_reader.h"
#include "vt/context/context.h"

#include "vt/runtime/runtime.h"

#include <cstdint>
#include <random>

namespace vt { namespace vrt { namespace collection { namespace lb {

void StatsMapLB::init(objgroup::proxy::Proxy<StatsMapLB> in_proxy) {
  proxy = in_proxy;
}

void StatsMapLB::runLB() {
  //
  // UH 2019/11/14
  // This routine is the main point of entry for load balancing.
  //

  std::cout << " RUNLB ... BaseLB::phase_ " << BaseLB::phase_
  << " phase_ " << phase_ << std::endl;

  std::cout << " node " << theContext()->getNode()
  << " map_changed " << balance::StatsLBReader::user_specified_map_changed_[phase_].size()
  << " proc_data " << balance::ProcStats::proc_data_[phase_].size()
  << std::endl;

  if (!balance::StatsLBReader::phase_changed_map_.vec_[phase_]) {
    std::cout << " >>> UH >>> Skip LB ... with phase_changed_map "
              << " ... node " << theContext()->getNode() << std::endl;
    return;
  }

  vtAssertExpr(balance::StatsLBReader::user_specified_map_changed_.size() > phase_);
  vtAssertExpr(balance::ProcStats::proc_comm_.size() > phase_);

  auto const &currentTempID = balance::ProcStats::proc_data_[phase_];
  auto const &nextPermID = balance::StatsLBReader::user_specified_map_changed_[phase_];

  auto const& in_load_stats = balance::ProcStats::proc_data_[phase_];
  auto const& in_comm_stats = balance::ProcStats::proc_comm_[phase_];

  auto const& this_node = theContext()->getNode();

  auto epoch = startMigrationCollective();
  theMsg()->pushEpoch(epoch);

  for (auto itmp : currentTempID) {
    auto iter = balance::ProcStats::proc_temp_to_perm_.find(itmp.first);
    if (iter == balance::ProcStats::proc_temp_to_perm_.end()) {
      vtAssert(false, "Temp ID must exist!");
    }
    auto myPermID = iter->first;
    if (nextPermID.count(myPermID) > 0) {
      migrateObjectTo(itmp.first, this_node);
    }
  }

  theMsg()->popEpoch(epoch);
  finishMigrationCollective();

}


}}}} /* end namespace vt::vrt::collection::lb */
