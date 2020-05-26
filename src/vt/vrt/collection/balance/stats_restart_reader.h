/*
//@HEADER
// *****************************************************************************
//
//                            stats_restart_reader.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_RESTART_READER_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_RESTART_READER_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/baselb/baselb_msgs.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"

#include <deque>
#include <map>
#include <string>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct StatsRestartReader : runtime::component::Component<StatsRestartReader> {
  using VecMsg = lb::TransferMsg<std::vector<balance::ElementIDType> >;

public:
  StatsRestartReader() = default;

  void setProxy(objgroup::proxy::Proxy<StatsRestartReader> in_proxy);

  static std::unique_ptr<StatsRestartReader> construct();

  std::string name() override { return "StatsRestartReader"; }

  void startup() override;

  void readStats(std::string const& fileName);

  std::vector<ElementIDType> const& getMoveList(PhaseType phase) const;

  std::deque<std::vector<ElementIDType>> const& getMigrationList() const;

  void clearMoveList(PhaseType phase);

  bool needsLB(PhaseType phase) const;

private:
  void inputStatsFile(
    std::string const& fileName,
    std::deque<std::set<ElementIDType>>& element_history
  );

  void createMigrationInfo(
    std::deque<std::set<ElementIDType>>& element_history
  );

  void gatherMsgs(VecMsg *msg);

  void scatterMsgs(VecMsg *msg);

private:
  /// \brief Vector counting the received messages per iteration
  /// \note Only node 0 will use this vector.
  std::vector<size_t> msgsReceived;

  /// \brief Queue for storing all the migrations per iteration.
  /// \note Only node 0 will use this queue.
  std::deque<std::map<ElementIDType, std::pair<NodeType, NodeType>>> totalMove;

  /// \brief Proxy for communicating the migration information
  objgroup::proxy::Proxy<StatsRestartReader> proxy_;

  /// \brief Queue of migrations for each iteration.
  /// \note At each iteration, a vector of length 2 times (# of migrations)
  /// is specified. The vector contains the "permanent" ID of the element
  /// to migrate followed by the node ID to migrate to.
  std::deque<std::vector<ElementIDType>> proc_move_list_;

  /// \brief Vector of booleans to indicate whether the user-specified
  /// map migrates elements for a specific iteration.
  std::vector<bool> proc_phase_runs_LB_;
};

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt {

extern vrt::collection::balance::StatsRestartReader* theStatsReader();

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_RESTART_READER_H*/
