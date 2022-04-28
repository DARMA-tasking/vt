/*
//@HEADER
// *****************************************************************************
//
//                                node_stats.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/node_stats.h"
#include "vt/vrt/collection/balance/baselb/baselb_msgs.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/runtime/runtime.h"
#include "vt/utils/json/json_appender.h"
#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/elm/elm_stats.h"

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
    stats_->node_comm_.erase(phase - look_back);
    stats_->node_subphase_comm_.erase(phase - look_back);
  }

  // Clear migrate lambdas and proxy lookup since LB is complete
  NodeStats::node_migrate_.clear();
  node_collection_lookup_.clear();
  node_objgroup_lookup_.clear();
}

ElementIDType NodeStats::getNextElm() {
  return next_elm_++;
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
  auto const file_name = theConfig()->getLBDataFileOut();
  auto const compress = theConfig()->vt_lb_stats_compress;

  vt_debug_print(
    normal, lb,
    "NodeStats::createStatsFile: file={}\n", file_name
  );

  auto const dir = theConfig()->vt_lb_stats_dir;
  // Node 0 creates the directory
  if (not created_dir_ and theContext()->getNode() == 0) {
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
    stat_writer_ = std::make_unique<JSONAppender>("phases", file_name, compress);
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

void NodeStats::fatalError() {
  // make flush occur on all stat data collected immediately
  stat_writer_ = nullptr;
}

void NodeStats::closeStatsFile() {
}

std::pair<ElementIDType, ElementIDType>
getRecvSendDirection(elm::CommKeyType const& comm) {
  switch (comm.cat_) {
  case elm::CommCategory::SendRecv:
  case elm::CommCategory::Broadcast:
    return std::make_pair(comm.toObj().id, comm.fromObj().id);

  case elm::CommCategory::NodeToCollection:
  case elm::CommCategory::NodeToCollectionBcast:
    return std::make_pair(comm.toObj().id, comm.fromNode());

  case elm::CommCategory::CollectionToNode:
  case elm::CommCategory::CollectionToNodeBcast:
    return std::make_pair(comm.toNode(), comm.fromObj().id);

  // Comm stats are not recorded for local operations
  // this case is just to avoid warning of not handled enum
  case elm::CommCategory::CollectiveToCollectionBcast:
  case elm::CommCategory::LocalInvoke:
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

void NodeStats::registerCollectionInfo(
  ElementIDStruct id, VirtualProxyType proxy,
  std::vector<uint64_t> const& index, MigrateFnType migrate_fn
) {
  // Add the index to the map
  stats_->node_idx_[id] = std::make_tuple(proxy, index);
  node_migrate_[id] = migrate_fn;
  node_collection_lookup_[id] = proxy;
}

void NodeStats::registerObjGroupInfo(
  ElementIDStruct id, ObjGroupProxyType proxy
) {
  stats_->node_objgroup_[id] = proxy;
  node_objgroup_lookup_[id] = proxy;
}

void NodeStats::addNodeStats(
  ElementIDStruct id, elm::ElementStats* in, SubphaseType focused_subphase
) {
  vt_debug_print(
    normal, lb,
    "NodeStats::addNodeStats: id={}\n", id
  );

  auto const phase = in->getPhase();
  auto const& total_load = in->getLoad(phase, focused_subphase);

  auto &phase_data = stats_->node_data_[phase];
  auto elm_iter = phase_data.find(id);
  vtAssert(elm_iter == phase_data.end(), "Must not exist");

  auto& subphase_times = in->getSubphaseTimes(phase);

  phase_data.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(LoadSummary{total_load, subphase_times})
  );

  auto const& comm = in->getComm(phase);
  auto &comm_data = stats_->node_comm_[phase];
  for (auto&& c : comm) {
    comm_data[c.first] += c.second;
  }

  auto const& subphase_comm = in->getSubphaseComm(phase);
  auto &subphase_comm_data = stats_->node_subphase_comm_[phase];
  for (SubphaseType i = 0; i < subphase_comm.size(); i++) {
    for (auto& sp : subphase_comm[i]) {
      subphase_comm_data[i][sp.first] += sp.second;
    }
  }

  in->updatePhase(1);

  auto model = theLBManager()->getLoadModel();
  in->releaseStatsFromUnneededPhases(phase, model->getNumPastPhasesNeeded());
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

ObjGroupProxyType NodeStats::getObjGroupProxyForElement(
  ElementIDStruct obj_id
) const {
  auto iter = node_objgroup_lookup_.find(obj_id);
  if (iter == node_objgroup_lookup_.end()) {
    return no_obj_group;
  }
  return iter->second;
}

}}}} /* end namespace vt::vrt::collection::balance */
