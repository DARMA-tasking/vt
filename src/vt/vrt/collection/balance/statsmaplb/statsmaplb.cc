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
#include "vt/vrt/collection/balance/statsmaplb/statsmaplb.h"
#include "vt/context/context.h"

#include <cstdint>
#include <random>

namespace vt { namespace vrt { namespace collection { namespace lb {

void StatsMapLB::init(objgroup::proxy::Proxy<StatsMapLB> in_proxy) {
  proxy = in_proxy;
}

void StatsMapLB::runLB() {
  vtAssertExpr(balance::ProcStats::user_specified_map_changed_.size() >= phase_);

  auto const& in_load_stats = balance::ProcStats::user_specified_map_changed_[phase_];
  auto const& in_comm_stats = balance::ProcStats::proc_comm_[phase_];

  // TODO: Check if we keep the import and the compute calls
  importProcessorData(in_load_stats, in_comm_stats);
  computeStatistics();

  loadPhaseChangedMap();

  startMigrationCollective();

  auto const& this_node = theContext()->getNode();

  vtAssertExpr(phase_changed_map_.size() >= phase_);

  if (phase_changed_map_[phase_]) {
    for (auto&& stat : *load_data) {
      auto const& obj = stat.first;
      auto const& load = stat.second;
      debug_print(
        lb, node,
        "\t StatsMapLB::migrating object to: obj={}, load={}, to_node={}\n",
        obj, load, this_node
      );
      migrateObjectTo(balance::ProcStats::proc_perm_to_temp_[obj], this_node);
    }
  }

  finishMigrationCollective();
}

void StatsMapLB::loadPhaseChangedMap() {
  auto const num_iters = balance::ProcStats::proc_data_.size() - 1;
  phase_changed_map_.resize(num_iters);

  for (size_t i = 0; i < num_iters; i++) {
    auto elms = balance::ProcStats::proc_data_.at(i);
    auto elmsNext = balance::ProcStats::proc_data_.at(i + 1);

    // elmsNext is different from elms if at least one of its element is different
    if (elmsNext.size() != elms.size()) {
      phase_changed_map_[i] = true;
    }

    auto elmsIte = elms.begin();
    auto elmsNextIte = elmsNext.begin();

    bool currentPhaseChanged = false;

    while ((elmsIte != elms.end()) && !currentPhaseChanged) {
     if ((elmsIte->first != elmsNextIte->first) ||
         (elmsIte->second != elmsNextIte->second)
         ) {
      currentPhaseChanged = true;
     }

     elmsIte++;
     elmsNextIte++;
    }

    phase_changed_map_[i] = currentPhaseChanged;
  }
}

}}}} /* end namespace vt::vrt::collection::lb */
