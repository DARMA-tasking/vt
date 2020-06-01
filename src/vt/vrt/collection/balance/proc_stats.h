/*
//@HEADER
// *****************************************************************************
//
//                                 proc_stats.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/lb_comm.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/timing/timing.h"

#include <vector>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct ProcStats {
  using MigrateFnType = std::function<void(NodeType)>;

public:
  template <typename ColT>
  static ElementIDType addProcStats(
    VirtualElmProxyType<ColT> const& elm_proxy, ColT* col_elm,
    PhaseType const& phase, TimeType const& time,
    std::vector<TimeType> const& subphase_time, CommMapType const& comm
  );

  static void clearStats();
  static void startIterCleanup();
  static void releaseLB();

  static void outputStatsFile();

private:
  static void createStatsFile();
  static void closeStatsFile();

public:
  static ElementIDType getNextElm();

  // @todo: make these private and friend appropriate classes
public:
  static ElementIDType next_elm_;
public:
  static std::vector<std::unordered_map<ElementIDType,TimeType>> proc_data_;
  static std::unordered_map<ElementIDType,MigrateFnType> proc_migrate_;
  static std::unordered_map<ElementIDType,ElementIDType> proc_temp_to_perm_;
  static std::unordered_map<ElementIDType,ElementIDType> proc_perm_to_temp_;
  static std::vector<CommMapType> proc_comm_;
private:
  static FILE* stats_file_;
  static bool created_dir_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#include "vt/vrt/collection/balance/proc_stats.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_H*/
