/*
//@HEADER
// *****************************************************************************
//
//                              proc_stats.impl.h
//                           DARMA Toolkit v. 1.0.0
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_IMPL_H
#define INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/proc_stats.h"

#include <vector>
#include <unordered_map>
#include <cassert>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename ColT>
/*static*/ ElementIDType ProcStats::addProcStats(
  VirtualElmProxyType<ColT> const& elm_proxy, ColT* col_elm,
  PhaseType const& phase, TimeType const& time,
  std::vector<TimeType> const& subphase_time, CommMapType const& comm
) {
  // A new temp ID gets assigned when a object is migrated into a node

  auto const temp_id = col_elm->temp_elm_id_;
  auto const perm_id = col_elm->stats_elm_id_;

  debug_print(
    lb, node,
    "ProcStats::addProcStats: temp_id={}, perm_id={}, phase={}, subphases={}, load={}\n",
    temp_id, perm_id, phase, subphase_time.size(), time
  );

  proc_data_.resize(phase + 1);
  auto elm_iter = proc_data_.at(phase).find(temp_id);
  vtAssert(elm_iter == proc_data_.at(phase).end(), "Must not exist");
  proc_data_.at(phase).emplace(
    std::piecewise_construct,
    std::forward_as_tuple(temp_id),
    std::forward_as_tuple(time)
  );

  proc_subphase_data_.resize(phase + 1);
  auto elm_subphase_iter = proc_subphase_data_.at(phase).find(temp_id);
  vtAssert(elm_subphase_iter == proc_subphase_data_.at(phase).end(), "Must not exist");
  proc_subphase_data_.at(phase).emplace(
    std::piecewise_construct,
    std::forward_as_tuple(temp_id),
    std::forward_as_tuple(subphase_time)
  );

  proc_comm_.resize(phase + 1);
  for (auto&& c : comm) {
    proc_comm_.at(phase)[c.first] += c.second;
  }

  proc_temp_to_perm_[temp_id] = perm_id;
  proc_perm_to_temp_[perm_id] = temp_id;

  auto migrate_iter = proc_migrate_.find(temp_id);
  if (migrate_iter == proc_migrate_.end()) {
    proc_migrate_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(temp_id),
      std::forward_as_tuple([col_elm](NodeType node){
        col_elm->migrate(node);
      })
    );
  }
  return temp_id;
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_IMPL_H*/
