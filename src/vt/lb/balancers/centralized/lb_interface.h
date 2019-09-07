/*
//@HEADER
// *****************************************************************************
//
//                                lb_interface.h
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

#if !defined INCLUDED_LB_BALANCERS_CENTRALIZED_LB_INTERFACE_H
#define INCLUDED_LB_BALANCERS_CENTRALIZED_LB_INTERFACE_H

#include "vt/config.h"
#include "vt/lb/lb_types.h"
#include "vt/lb/lb_types_internal.h"
#include "vt/lb/migration/migrate.h"

#include <vector>
#include <unordered_map>

namespace vt { namespace lb { namespace centralized {

struct CentralLB {
  using LoadStatsType = ::vt::lb::ProcContainerType;
  using MigrateInfoType = ::vt::lb::MigrateInfo;

  CentralLB(NodeType const& node, NodeType const& central_node)
    : this_node_(node), central_node_(central_node)
  { }
  CentralLB(CentralLB const&) = delete;
  CentralLB(CentralLB&&) = delete;

  virtual void loadStatistics(LoadStatsType const& stats) = 0;
  virtual void sync() = 0;
  virtual void runLB() = 0;
  virtual void notifyMigration(
    NodeType const& from, NodeType const& to, LBEntityType const& entity
  ) = 0;
  virtual void notifyMigrationList(MigrateInfoType const& migrate_info) = 0;
  virtual void finishedMigrations() = 0;

  NodeType getCentralNode() const { return central_node_; }
  NodeType getThisNode() const { return this_node_; }

private:
  NodeType const this_node_;
  NodeType const central_node_;
};

}}} /* end namespace vt::lb::centralized */

#endif /*INCLUDED_LB_BALANCERS_CENTRALIZED_LB_INTERFACE_H*/
