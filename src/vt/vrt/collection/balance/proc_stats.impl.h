/*
//@HEADER
// ************************************************************************
//
//                          proc_stats.impl.h
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
  PhaseType const& phase, TimeType const& time
) {
  // Always assign a new element ID so the node is correct (for now)
  col_elm->temp_elm_id_ = ProcStats::getNextElm();

  auto const temp_id = col_elm->temp_elm_id_;

  debug_print(
    lb, node,
    "ProcStats::addProcStats: temp_id={}, perm_id={}, phase={}, load={}\n",
    col_elm->temp_elm_id_, col_elm->stats_elm_id_, phase, time
  );

  proc_data_.resize(phase + 1);
  auto elm_iter = proc_data_.at(phase).find(temp_id);
  vtAssert(elm_iter == proc_data_.at(phase).end(), "Must not exist");
  proc_data_.at(phase).emplace(
    std::piecewise_construct,
    std::forward_as_tuple(temp_id),
    std::forward_as_tuple(time)
  );
  proc_temp_to_perm_[temp_id] = col_elm->stats_elm_id_;
  auto migrate_iter = proc_migrate_.find(temp_id);
  if (migrate_iter == proc_migrate_.end()) {
    proc_migrate_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(temp_id),
      std::forward_as_tuple([elm_proxy,col_elm](NodeType node){
        col_elm->migrate(node);
      })
    );
  }
  return temp_id;
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_IMPL_H*/
