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
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/args.h"
#include "vt/runtime/runtime.h"

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>

#include "fmt/format.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/
std::vector<std::unordered_map<ElementIDType,TimeType>>
  ProcStats::proc_data_ = {};

/*static*/ std::vector<CommMapType> ProcStats::proc_comm_ = {};

/*static*/
std::unordered_map<ElementIDType,ProcStats::MigrateFnType>
  ProcStats::proc_migrate_ = {};

/*static*/ std::unordered_map<ElementIDType,ElementIDType>
  ProcStats::proc_temp_to_perm_ =  {};

/*static*/ std::unordered_map<ElementIDType,ElementIDType>
  ProcStats::proc_perm_to_temp_ =  {};

/*static*/ ElementIDType ProcStats::next_elm_ = 1;

/*static*/ FILE* ProcStats::stats_file_ = nullptr;

/*static*/ bool ProcStats::created_dir_ = false;

/*static*/ std::deque< std::vector<ElementIDType> >
  ProcStats::proc_move_list_ = {};

/*static*/ std::vector< bool > ProcStats::proc_phase_runs_LB_ = {};

/*static*/ void ProcStats::clearStats() {
  ProcStats::proc_comm_.clear();
  ProcStats::proc_data_.clear();
  ProcStats::proc_migrate_.clear();
  ProcStats::proc_temp_to_perm_.clear();
  ProcStats::proc_perm_to_temp_.clear();
  next_elm_ = 1;
}

/*static*/ void ProcStats::startIterCleanup() {
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

/*static*/ ElementIDType ProcStats::getNextElm() {
  auto const& this_node = theContext()->getNode();
  auto elm = next_elm_++;
  return (elm << 32) | this_node;
}

/*static*/ void ProcStats::releaseLB() {
  using MsgType = CollectionPhaseMsg;
  auto msg = makeMessage<MsgType>();
  theMsg()->broadcastMsg<MsgType,CollectionManager::releaseLBPhase>(msg.get());
  CollectionManager::releaseLBPhase(msg.get());
}

/*static*/ void ProcStats::createStatsFile() {
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

/*static*/ void ProcStats::closeStatsFile() {
  if (stats_file_) {
    fclose(stats_file_);
    stats_file_  = nullptr;
  }
}

/*static*/ void ProcStats::outputStatsFile() {
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
