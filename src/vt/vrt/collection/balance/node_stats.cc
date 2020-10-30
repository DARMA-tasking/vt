/*
//@HEADER
// *****************************************************************************
//
//                                node_stats.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/node_stats.h"
#include "vt/vrt/collection/balance/baselb/baselb_msgs.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/runtime/runtime.h"

#include <vector>
#include <unordered_map>
#include <cstdio>
#include <sys/stat.h>

#include "fmt/format.h"

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

ElementIDType NodeStats::tempToPerm(ElementIDType temp_id) const {
  auto iter = node_temp_to_perm_.find(temp_id);
  if (iter == node_temp_to_perm_.end()) {
    return no_element_id;
  }
  return iter->second;
}

ElementIDType NodeStats::permToTemp(ElementIDType perm_id) const {
  auto iter = node_perm_to_temp_.find(perm_id);
  if (iter == node_perm_to_temp_.end()) {
    return no_element_id;
  }
  return iter->second;
}

bool NodeStats::hasObjectToMigrate(ElementIDType obj_id) const {
  auto iter = node_migrate_.find(obj_id);
  return iter != node_migrate_.end();
}

bool NodeStats::migrateObjTo(ElementIDType obj_id, NodeType to_node) {
  auto iter = node_migrate_.find(obj_id);
  if (iter == node_migrate_.end()) {
    return false;
  }

  auto migrate_fn = iter->second;
  migrate_fn(to_node);

  return true;
}

void NodeStats::clearStats() {
  NodeStats::local_stats_.clear();
  NodeStats::node_migrate_.clear();
  NodeStats::node_temp_to_perm_.clear();
  NodeStats::node_perm_to_temp_.clear();
  next_elm_ = 1;
}

void NodeStats::startIterCleanup(PhaseType phase, unsigned int look_back) {
  // Create migrate lambdas and temp to perm map since LB is complete
  NodeStats::local_stats_.clear();
  NodeStats::node_migrate_.clear();
  NodeStats::node_temp_to_perm_.clear();
  NodeStats::node_perm_to_temp_.clear();
  node_collection_lookup_.clear();
}

ElementIDType NodeStats::getNextElm() {
  auto const& this_node = theContext()->getNode();
  auto elm = next_elm_++;
  return (elm << 32) | this_node;
}

void NodeStats::initialize() {
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
    lb, node,
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

  stats_file_ = fopen(file_name.c_str(), "w+");
  vtAssertExpr(stats_file_ != nullptr);
}

void NodeStats::finalize() {
  // If statistics are enabled, close output file and clear stats
#if vt_check_enabled(lblite)
  if (theConfig()->vt_lb_stats) {
    closeStatsFile();
    clearStats();
  }
#endif
}

void NodeStats::closeStatsFile() {
  if (stats_file_) {
    fclose(stats_file_);
    stats_file_ = nullptr;
  }
}

std::pair<ElementIDType, ElementIDType>
getRecvSendDirection(CommKeyType const& comm) {
  switch (comm.cat_) {
  case CommCategory::SendRecv:
  case CommCategory::Broadcast:
    return std::make_pair(comm.toObj(), comm.fromObj());

  case CommCategory::NodeToCollection:
  case CommCategory::NodeToCollectionBcast:
    return std::make_pair(comm.toObj(), comm.fromNode());

  case CommCategory::CollectionToNode:
  case CommCategory::CollectionToNodeBcast:
    return std::make_pair(comm.toNode(), comm.fromObj());

  // Comm stats are not recorded for collective bcast
  // this case is just to avoid warning of not handled enum
  case CommCategory::CollectiveToCollectionBcast:
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

  vtAssertExpr(stats_file_ != nullptr);

  vt_print(lb, "NodeStats::outputStatsForPhase: phase={}\n", phase);

  for (auto&& elm : local_stats_) {
    ElementIDType id = elm.first;
    ElementStats *stats = elm.second;

    TimeType time = stats->getLoad(phase);
    const auto& subphase_times = stats->getSubphaseLoads(phase);
    size_t subphases = subphase_times.size();

    auto obj_str = fmt::format("{},{},{},{},[", phase, id, time, subphases);

    for (size_t s = 0; s < subphases; s++) {
      if (s > 0) {
        obj_str += ",";
      }

      obj_str += std::to_string(subphase_times[s]);
    }

    obj_str += "]\n";

    fprintf(stats_file_, "%s", obj_str.c_str());
  }

  CommMapType comm_data;
  //std::unordered_map<SubphaseType, CommMapType> subphase_comm_data;

  for (auto&& elm : local_stats_) {
    ElementStats *stats = elm.second;

    auto const& comm = stats->getComm(phase);
    for (auto&& c : comm) {
      comm_data[c.first] += c.second;
    }

    //auto const& subphase_comm = stats->getSubphaseComm(phase);
    //for (SubphaseType i = 0; i < subphase_comm.size(); i++) {
    //  for (auto& sp : subphase_comm[i]) {
    //    subphase_comm_data[i][sp.first] += sp.second;
    //  }
    //}
  }

  for (auto&& elm : comm_data) {
    using E = typename std::underlying_type<CommCategory>::type;

    auto const& comm = elm.first;
    auto const recvSend = getRecvSendDirection(comm);
    auto const cat = static_cast<E>(comm.cat_);
    auto obj_str = fmt::format(
      "{},{},{},{},{}\n", phase, recvSend.first, recvSend.second,
      elm.second.bytes, cat
    );
    fprintf(stats_file_, "%s", obj_str.c_str());
  }

  fflush(stats_file_);
}

ElementIDType NodeStats::addNodeStats(
  Migratable* col_elm,
  PhaseType const& phase, ElementStats *stats
) {
  // A new temp ID gets assigned when a object is migrated into a node

  auto const temp_id = col_elm->temp_elm_id_;
  auto const perm_id = col_elm->stats_elm_id_;

  vt_debug_print(
    lb, node,
    "NodeStats::addNodeStats: temp_id={}, perm_id={}, phase={}, subphases={}, load={}\n",
    temp_id, perm_id, phase, stats->getSubphaseLoads(phase).size(), stats->getLoad(phase)
  );

  local_stats_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(perm_id),
    std::forward_as_tuple(stats)
  );

  node_temp_to_perm_[temp_id] = perm_id;
  node_perm_to_temp_[perm_id] = temp_id;

  auto migrate_iter = node_migrate_.find(temp_id);
  if (migrate_iter == node_migrate_.end()) {
    node_migrate_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(temp_id),
      std::forward_as_tuple([col_elm](NodeType node){
        col_elm->migrate(node);
      })
    );
  }

  auto const col_proxy = col_elm->getProxy();
  node_collection_lookup_[temp_id] = col_proxy;

  return temp_id;
}

VirtualProxyType NodeStats::getCollectionProxyForElement(
  ElementIDType temp_id
) const {
  auto iter = node_collection_lookup_.find(temp_id);
  if (iter == node_collection_lookup_.end()) {
    return no_vrt_proxy;
  }
  return iter->second;
}

}}}} /* end namespace vt::vrt::collection::balance */
