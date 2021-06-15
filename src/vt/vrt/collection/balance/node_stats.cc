/*
//@HEADER
// *****************************************************************************
//
//                                node_stats.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/node_stats.h"
#include "vt/vrt/collection/balance/baselb/baselb_msgs.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/runtime/runtime.h"
#include "vt/utils/json/json_appender.h"
#include "vt/vrt/collection/balance/stats_data.h"

#include <vector>
#include <unordered_map>
#include <cstdio>
#include <sys/stat.h>

#include <fmt/core.h>

namespace vt { namespace vrt { namespace collection { namespace balance {

void NodeStats::setProxy(objgroup::proxy::Proxy<NodeStats> in_proxy) {
  proxy_ = in_proxy;
}

/*static*/ std::unique_ptr<NodeStats> NodeStats::construct() {
  auto ptr = std::make_unique<NodeStats>();
  auto proxy = theObjGroup()->makeCollective<NodeStats>(ptr.get());
  proxy.get()->setProxy(proxy);
  return ptr;
}

bool NodeStats::hasObjectToMigrate(ElementIDStruct obj_id) const {
  auto iter = node_migrate_.find(obj_id);
  return iter != node_migrate_.end();
}

bool NodeStats::migrateObjTo(ElementIDStruct obj_id, NodeType to_node) {
  auto iter = node_migrate_.find(obj_id);
  if (iter == node_migrate_.end()) {
    return false;
  }

  auto migrate_fn = iter->second;
  migrate_fn(to_node);

  return true;
}

std::unordered_map<PhaseType, LoadMapType> const*
NodeStats::getNodeLoad() const {
  return &stats_->node_data_;
}

std::unordered_map<PhaseType, SubphaseLoadMapType> const*
NodeStats::getNodeSubphaseLoad() const {
  return &stats_->node_subphase_data_;
}

std::unordered_map<PhaseType, CommMapType> const* NodeStats::getNodeComm() const {
  return &stats_->node_comm_;
}

std::unordered_map<PhaseType, std::unordered_map<SubphaseType, CommMapType>> const* NodeStats::getNodeSubphaseComm() const {
  return &stats_->node_subphase_comm_;
}

void NodeStats::clearStats() {
  stats_->clear();
  node_migrate_.clear();
  next_elm_ = 1;
}

void NodeStats::startIterCleanup(PhaseType phase, unsigned int look_back) {
  if (phase >= look_back) {
    stats_->node_data_.erase(phase - look_back);
    stats_->node_subphase_data_.erase(phase - look_back);
    stats_->node_comm_.erase(phase - look_back);
    stats_->node_subphase_comm_.erase(phase - look_back);
  }

  // Clear migrate lambdas and proxy lookup since LB is complete
  NodeStats::node_migrate_.clear();
  node_collection_lookup_.clear();
}

ElementIDStruct NodeStats::getNextElm() {
  auto const& this_node = theContext()->getNode();
  auto id = (next_elm_++ << 32) | this_node;
  ElementIDStruct elm{id, this_node, this_node};
  return elm;
}

void NodeStats::initialize() {
  stats_ = std::make_unique<StatsData>();

#if vt_check_enabled(lblite)
  if (theConfig()->vt_lb_stats) {
    theNodeStats()->createStatsFile();
  }
#endif
}

void NodeStats::createStatsFile() {
  auto const node = theContext()->getNode();
  auto const base_file = theConfig()->vt_lb_stats_file;
  auto const dir = theConfig()->vt_lb_stats_dir;
  auto const file = fmt::format("{}.{}.out", base_file, node);
  auto const file_name = fmt::format("{}/{}", dir, file);

  vt_debug_print(
    normal, lb,
    "NodeStats::createStatsFile: file={}\n", file_name
  );

  // Node 0 creates the directory
  if (not created_dir_ and node == 0) {
    mkdir(dir.c_str(), S_IRWXU);
    created_dir_ = true;
  }

  // Barrier: wait for node 0 to create directory before trying to put a file in
  // the stats destination directory
  if (curRT) {
    curRT->systemSync();
  } else {
    // Something is wrong
    vtAssert(false, "Trying to dump stats when VT runtime is deallocated?");
  }

  using JSONAppender = util::json::Appender<std::ofstream>;

  if (not stat_writer_) {
    stat_writer_ = std::make_unique<JSONAppender>("phases", file_name, true);
  }
}

void NodeStats::finalize() {
  stat_writer_ = nullptr;

  // If statistics are enabled, close output file and clear stats
#if vt_check_enabled(lblite)
  if (theConfig()->vt_lb_stats) {
    clearStats();
  }
#endif
}

void NodeStats::closeStatsFile() {
}

std::pair<ElementIDType, ElementIDType>
getRecvSendDirection(CommKeyType const& comm) {
  switch (comm.cat_) {
  case CommCategory::SendRecv:
  case CommCategory::Broadcast:
    return std::make_pair(comm.toObj().id, comm.fromObj().id);

  case CommCategory::NodeToCollection:
  case CommCategory::NodeToCollectionBcast:
    return std::make_pair(comm.toObj().id, comm.fromNode());

  case CommCategory::CollectionToNode:
  case CommCategory::CollectionToNodeBcast:
    return std::make_pair(comm.toNode(), comm.fromObj().id);

  // Comm stats are not recorded for local operations
  // this case is just to avoid warning of not handled enum
  case CommCategory::CollectiveToCollectionBcast:
  case CommCategory::LocalInvoke:
    return std::make_pair(ElementIDType{}, ElementIDType{});
  }

  vtAssert(false, "Invalid balance::CommCategory enum value");
  return std::make_pair(ElementIDType{}, ElementIDType{});
}

void NodeStats::outputStatsForPhase(PhaseType phase) {
  // Statistics output when LB is enabled and appropriate flag is enabled
  if (!theConfig()->vt_lb_stats) {
    return;
  }

  vt_print(lb, "NodeStats::outputStatsForPhase: phase={}\n", phase);

  using JSONAppender = util::json::Appender<std::ofstream>;

  auto j = stats_->toJson(phase);
  auto writer = static_cast<JSONAppender*>(stat_writer_.get());
  writer->addElm(*j);
}

ElementIDStruct NodeStats::addNodeStats(
  Migratable* col_elm,
  PhaseType const& phase, TimeType const& time,
  std::vector<TimeType> const& subphase_time,
  CommMapType const& comm, std::vector<CommMapType> const& subphase_comm
) {
  // The ID struct is modified when a object is migrated into a node

  auto const obj_id = col_elm->elm_id_;

  vt_debug_print(
    normal, lb,
    "NodeStats::addNodeStats: obj_id={}, phase={}, subphases={}, load={}\n",
    obj_id, phase, subphase_time.size(), time
  );

  auto &phase_data = stats_->node_data_[phase];
  auto elm_iter = phase_data.find(obj_id);
  vtAssert(elm_iter == phase_data.end(), "Must not exist");
  phase_data.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(obj_id),
    std::forward_as_tuple(time)
  );

  auto &subphase_data = stats_->node_subphase_data_[phase];
  auto elm_subphase_iter = subphase_data.find(obj_id);
  vtAssert(elm_subphase_iter == subphase_data.end(), "Must not exist");
  subphase_data.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(obj_id),
    std::forward_as_tuple(subphase_time)
  );

  auto &comm_data = stats_->node_comm_[phase];
  for (auto&& c : comm) {
    comm_data[c.first] += c.second;
  }

  auto &subphase_comm_data = stats_->node_subphase_comm_[phase];
  for (SubphaseType i = 0; i < subphase_comm.size(); i++) {
    for (auto& sp : subphase_comm[i]) {
      subphase_comm_data[i][sp.first] += sp.second;
    }
  }

  auto migrate_iter = node_migrate_.find(obj_id);
  if (migrate_iter == node_migrate_.end()) {
    node_migrate_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(obj_id),
      std::forward_as_tuple([col_elm](NodeType node){
        col_elm->migrate(node);
      })
    );
  }

  auto const col_proxy = col_elm->getProxy();
  node_collection_lookup_[obj_id] = col_proxy;

  return obj_id;
}

VirtualProxyType NodeStats::getCollectionProxyForElement(
  ElementIDStruct obj_id
) const {
  auto iter = node_collection_lookup_.find(obj_id);
  if (iter == node_collection_lookup_.end()) {
    return no_vrt_proxy;
  }
  return iter->second;
}

}}}} /* end namespace vt::vrt::collection::balance */
