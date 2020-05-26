/*
//@HEADER
// *****************************************************************************
//
//                                proc_stats.cc
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
#include "vt/vrt/collection/balance/proc_stats.h"
#include "vt/vrt/collection/balance/proc_stats.util.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/args.h"
#include "vt/runtime/runtime.h"

#include <vector>
#include <unordered_map>
#include <cstdio>
#include <sys/stat.h>

#include "fmt/format.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

void ProcStats::setProxy(objgroup::proxy::Proxy<ProcStats> in_proxy) {
  proxy_ = in_proxy;
}

/*static*/ std::unique_ptr<ProcStats> ProcStats::construct() {
  auto ptr = std::make_unique<ProcStats>();
  auto proxy = theObjGroup()->makeCollective<ProcStats>(ptr.get());
  proxy.get()->setProxy(proxy);
  return ptr;
}

ElementIDType ProcStats::tempToPerm(ElementIDType temp_id) const {
  auto iter = proc_temp_to_perm_.find(temp_id);
  if (iter == proc_temp_to_perm_.end()) {
    return no_element_id;
  }
  return iter->second;
}

ElementIDType ProcStats::permToTemp(ElementIDType perm_id) const {
  auto iter = proc_perm_to_temp_.find(perm_id);
  if (iter == proc_perm_to_temp_.end()) {
    return no_element_id;
  }
  return iter->second;
}

bool ProcStats::hasObjectToMigrate(ElementIDType obj_id) const {
  auto iter = proc_migrate_.find(obj_id);
  return iter != proc_migrate_.end();
}

bool ProcStats::migrateObjTo(ElementIDType obj_id, NodeType to_node) {
  auto iter = proc_migrate_.find(obj_id);
  if (iter == proc_migrate_.end()) {
    return false;
  }

  auto migrate_fn = iter->second;
  migrate_fn(to_node);

  return true;
}

ProcStats::LoadMapType const&
ProcStats::getProcLoad(PhaseType phase) const {
  vtAssert(proc_data_.size() > phase, "Phase must exist in load data");
  return proc_data_.at(phase);
}

CommMapType const& ProcStats::getProcComm(PhaseType phase) const {
  vtAssert(proc_comm_.size() > phase, "Phase must exist in comm data");
  return proc_comm_.at(phase);

}

void ProcStats::clearStats() {
  ProcStats::proc_comm_.clear();
  ProcStats::proc_data_.clear();
  ProcStats::proc_migrate_.clear();
  ProcStats::proc_temp_to_perm_.clear();
  ProcStats::proc_perm_to_temp_.clear();
  next_elm_ = 1;
}

void ProcStats::startIterCleanup() {
  // Convert the temp ID proc_data_ for the last iteration into perm ID for
  // stats output
  auto const phase = proc_data_.size() - 1;
  auto const prev_data = std::move(proc_data_[phase]);
  std::unordered_map<ElementIDType,TimeType> new_data;
  for (auto& elm : prev_data) {
    auto iter = proc_temp_to_perm_.find(elm.first);
    vtAssert(iter != proc_temp_to_perm_.end(), "Temp ID must exist");
    auto perm_id = iter->second;
    new_data[perm_id] = elm.second;
  }
  proc_data_[phase] = std::move(new_data);

  // Create migrate lambdas and temp to perm map since LB is complete
  ProcStats::proc_migrate_.clear();
  ProcStats::proc_temp_to_perm_.clear();
  ProcStats::proc_perm_to_temp_.clear();
}

ElementIDType ProcStats::getNextElm() {
  auto const& this_node = theContext()->getNode();
  auto elm = next_elm_++;
  return (elm << 32) | this_node;
}

void ProcStats::releaseLB() {
  using MsgType = CollectionPhaseMsg;
  auto msg = makeMessage<MsgType>();
  theMsg()->broadcastMsg<MsgType,CollectionManager::releaseLBPhase>(msg.get());
  CollectionManager::releaseLBPhase(msg.get());
}

void ProcStats::createStatsFile() {
  using ArgType = vt::arguments::ArgConfig;
  auto const node = theContext()->getNode();
  auto const base_file = std::string(ArgType::vt_lb_stats_file);
  auto const dir = std::string(ArgType::vt_lb_stats_dir);
  auto const file = fmt::format("{}.{}.out", base_file, node);
  auto const file_name = fmt::format("{}/{}", dir, file);

  debug_print(
    lb, node,
    "ProcStats: createStatsFile file={}\n", file_name
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
}

void ProcStats::closeStatsFile() {
  if (stats_file_) {
    fclose(stats_file_);
    stats_file_  = nullptr;
  }
}

void ProcStats::outputStatsFile() {
  if (stats_file_ == nullptr) {
    createStatsFile();
  }

  vtAssertExpr(stats_file_ != nullptr);

  auto const num_iters = ProcStats::proc_data_.size();

  vt_print(lb, "outputStatsFile: file={}, iter={}\n", print_ptr(stats_file_), num_iters);

  for (size_t i = 0; i < num_iters; i++) {
    for (auto&& elm : ProcStats::proc_data_.at(i)) {
      auto obj_str = fmt::format("{},{},{}\n", i, elm.first, elm.second);
      fprintf(stats_file_, "%s", obj_str.c_str());
    }
    for (auto&& elm : ProcStats::proc_comm_.at(i)) {
      using E = typename std::underlying_type<CommCategory>::type;

      auto const& key = elm.first;
      auto const& val = elm.second;
      auto const cat = static_cast<E>(key.cat_);

      if (
        key.cat_ == CommCategory::SendRecv or
        key.cat_ == CommCategory::Broadcast
      ) {
        auto const to   = key.toObj();
        auto const from = key.fromObj();
        auto obj_str = fmt::format("{},{},{},{},{}\n", i, to, from, val, cat);
        fprintf(stats_file_, "%s", obj_str.c_str());
      } else if (
        key.cat_ == CommCategory::NodeToCollection or
        key.cat_ == CommCategory::NodeToCollectionBcast
      ) {
        auto const to   = key.toObj();
        auto const from = key.fromNode();
        auto obj_str = fmt::format("{},{},{},{},{}\n", i, to, from, val, cat);
        fprintf(stats_file_, "%s", obj_str.c_str());
      } else if (
        key.cat_ == CommCategory::CollectionToNode or
        key.cat_ == CommCategory::CollectionToNodeBcast
      ) {
        auto const to   = key.toNode();
        auto const from = key.fromObj();
        auto obj_str = fmt::format("{},{},{},{},{}\n", i, to, from, val, cat);
        fprintf(stats_file_, "%s", obj_str.c_str());
      } else {
        vtAssert(false, "Invalid balance::CommCategory enum value");
      }
    }
  }
  fflush(stats_file_);

  closeStatsFile();
}

}}}} /* end namespace vt::vrt::collection::balance */
