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

#include <iostream>
#include <unordered_map>


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
    return;
  }

  vtAssertExpr(balance::StatsLBReader::user_specified_map_changed_.size() > phase_);
  vtAssertExpr(balance::ProcStats::proc_comm_.size() > phase_);

  auto const &currentTempID = balance::ProcStats::proc_data_[phase_]; /* std::unordered_map */
  auto const &nextPermID = balance::StatsLBReader::user_specified_map_changed_[phase_];

  auto const& in_load_stats = balance::ProcStats::proc_data_[phase_];
  auto const& in_comm_stats = balance::ProcStats::proc_comm_[phase_];

  auto const& this_node = theContext()->getNode();

  if (this_node == 0) {
    std::cout << " ----------------- \n";
    for (auto itmp : balance::ProcStats::proc_temp_to_perm_) {
      std::cout << " node " << this_node << " tempID " << itmp.first << " permID " << itmp.second
                << std::endl;
    }
    std::cout << " -0-0-0-0-0-0-0-0-0-0- \n";
  }

  std::set<balance::ElementIDType> currentPermID;
  for (auto itmp : currentTempID) {
    auto iter = balance::ProcStats::proc_temp_to_perm_.find(itmp.first);
    if (iter == balance::ProcStats::proc_temp_to_perm_.end()) {
      vtAssert(false, "Temp ID must exist!");
    }
    currentPermID.insert(iter->second);
  }

  if (this_node == 0) {
    std::cout << " ----------------- \n";
    for (auto itmp : currentPermID) {
      std::cout << " node " << this_node << " PERM_ID " << itmp
                << std::endl;
    }
    std::cout << " -0-0-0-0-0-0-0-0-0-0- \n";
  }

  //// UH
  /// E_{t}   = Stay_{t} + Leave_{t}
  /// E_{t+1} = Stay_{t} + Come_{t}
  /// What if I make the next temp ID array?
  /// Migrate the (future) tempID to myself
  //// UH

  auto epoch = startMigrationCollective();
  theMsg()->pushEpoch(epoch);

  balance::ElementIDType nextElemID = balance::ProcStats::next_elm_;
  for (auto itmp : nextPermID) {
    auto iter = currentPermID.find(itmp.first);
    if (iter == currentPermID.end()) {
      auto elm = nextElemID++;
      balance::ElementIDType newTmpID = (elm << 32) | this_node;
      balance::ProcStats::proc_perm_to_temp_.insert(std::make_pair(itmp.first, newTmpID));
      balance::ProcStats::proc_temp_to_perm_.insert(std::make_pair(newTmpID, itmp.first));
      balance::ProcStats::proc_migrate_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(newTmpID),
          std::forward_as_tuple([col_elm](NodeType node){
            col_elm->migrate(node);
          })
        );
      }
      if (this_node == 0) {
  std::cout << " INSERT >> node " << this_node << " tempID " << newTmpID << " permID " << itmp.first
                << " size " << balance::ProcStats::proc_temp_to_perm_.size() << std::endl;
      }
      migrateObjectTo(newTmpID, this_node);
    }
    else {
      auto iter = balance::ProcStats::proc_perm_to_temp_.find(itmp.first);
      migrateObjectTo(iter->first, this_node);
    }
  }


  ///
  /// UH -- TODO Need to loop on the list of next items
  /// UH -- Previous is only working for the elements that do not move.
  ///

  theMsg()->popEpoch(epoch);
  finishMigrationCollective();

}


}}}} /* end namespace vt::vrt::collection::lb */
