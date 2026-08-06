/*
//@HEADER
// *****************************************************************************
//
//                                temperedlb.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#include "vt/vrt/collection/balance/temperedlb/temperedlb.h"

#include "vt/configs/error/config_assert.h"
#include "vt/elm/elm_comm.h"
#include "vt/vrt/collection/balance/model/load_model.h"

#if vt_check_enabled(lblite)
#  include <comm/comm/vt/comm_vt.h>
#  include <vt-lb/algo/temperedlb/configuration.h>
#  include <vt-lb/algo/temperedlb/temperedlb.h>
#  include <vt-lb/algo/temperedlb/transfer_util.h>
#  include <vt-lb/model/Communication.h>
#  include <vt-lb/model/PhaseData.h>
#  include <vt-lb/model/SharedBlock.h>
#  include <vt-lb/model/Task.h>
#endif

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

namespace vt { namespace vrt { namespace collection { namespace lb {

namespace {

template <typename T>
std::optional<double> numericValue(T const& value) {
  if (auto const* v = std::get_if<double>(&value)) {
    return *v;
  }
  if (auto const* v = std::get_if<int>(&value)) {
    return static_cast<double>(*v);
  }
  return std::nullopt;
}

} // end anonymous namespace

/*static*/ std::unordered_map<std::string, std::string>
TemperedLB::getInputKeysWithHelp() {
  return {
    {"fanout", "Values: <positive integer>\nDefault: 2\nRanks contacted during information propagation.\n"},
    {"rounds", "Values: <positive integer>\nDefault: ceil(sqrt(log2(num_ranks)))\nInformation-propagation rounds.\n"},
    {"iters", "Values: <positive integer>\nDefault: 10\nBalancing iterations per trial.\n"},
    {"trials", "Values: <positive integer>\nDefault: 1\nIndependent trials.\n"},
    {"criterion", "Values: {Grapevine, ModifiedGrapevine}\nDefault: ModifiedGrapevine\n"},
    {"ordering", "Values: {Arbitrary, ElmID, FewestMigrations, SmallObjects, LargestObjects}\nDefault: ElmID\n"},
    {"cmf", "Values: {Original, NormByMax, NormByMaxExcludeIneligible}\nDefault: Original\n"},
    {"deterministic", "Values: {true, false}\nDefault: true\n"},
    {"seed", "Values: <integer>\nDefault: 29\nRandom seed used in deterministic mode.\n"},
    {"alpha", "Values: <double>\nDefault: 1.0\nCompute-load coefficient.\n"},
    {"beta", "Values: <double>\nDefault: 0.0\nInter-rank communication coefficient.\n"},
    {"gamma", "Values: <double>\nDefault: 0.0\nIntra-rank communication coefficient.\n"},
    {"delta", "Values: <double>\nDefault: 0.0\nShared-memory communication coefficient.\n"},
    {"memory_threshold", "Values: <double>\nDefault: 0.0\nMaximum memory available per rank.\n"},
    {"converge_tolerance", "Values: <double>\nDefault: 0.01\nRelative convergence tolerance.\n"}
  };
}

void TemperedLB::inputParams(balance::ConfigEntry* config) {
  std::vector<std::string> allowed;
  for (auto const& [key, help] : getInputKeysWithHelp()) {
    (void)help;
    allowed.push_back(key);
  }
  config->checkAllowedKeys(allowed);

  auto const num_nodes = theContext()->getNumNodes();
  auto const default_rounds = std::max<int32_t>(
    1,
    static_cast<int32_t>(
      std::ceil(std::sqrt(std::log(std::max<NodeType>(num_nodes, 2)) / std::log(2.0)))
    )
  );

  fanout_ = config->getOrDefault<int32_t>("fanout", fanout_);
  rounds_ = config->getOrDefault<int32_t>("rounds", default_rounds);
  num_iters_ = config->getOrDefault<int32_t>("iters", num_iters_);
  num_trials_ = config->getOrDefault<int32_t>("trials", num_trials_);
  seed_ = config->getOrDefault<int32_t>("seed", seed_);
  deterministic_ = config->getOrDefault<bool>("deterministic", deterministic_);
  alpha_ = config->getOrDefault<double>("alpha", alpha_);
  beta_ = config->getOrDefault<double>("beta", beta_);
  gamma_ = config->getOrDefault<double>("gamma", gamma_);
  delta_ = config->getOrDefault<double>("delta", delta_);
  memory_threshold_ = config->getOrDefault<double>(
    "memory_threshold", memory_threshold_
  );
  converge_tolerance_ = config->getOrDefault<double>(
    "converge_tolerance", converge_tolerance_
  );
  criterion_ = config->getOrDefault<std::string>("criterion", criterion_);
  ordering_ = config->getOrDefault<std::string>("ordering", ordering_);
  cmf_ = config->getOrDefault<std::string>("cmf", cmf_);

  vtAbortIf(fanout_ < 1, "TemperedLB fanout must be positive");
  vtAbortIf(rounds_ < 1, "TemperedLB rounds must be positive");
  vtAbortIf(num_iters_ < 1, "TemperedLB iters must be positive");
  vtAbortIf(num_trials_ < 1, "TemperedLB trials must be positive");
}

void TemperedLB::runLB([[maybe_unused]] LoadType total_load) {
#if vt_check_enabled(lblite)
  using ExternalLB = vt_lb::algo::temperedlb::TemperedLB<comm::CommVT>;
  using ExternalConfig = vt_lb::algo::temperedlb::Configuration;
  using ExternalTransferUtil = vt_lb::algo::temperedlb::TransferUtil;
  using ExternalTaskType = vt_lb::model::TaskType;

  auto const this_node = theContext()->getNode();
  auto const when = balance::PhaseOffset{
    balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE
  };

  auto phase_data = std::make_unique<vt_lb::model::PhaseData>(this_node);
  std::unordered_map<ExternalTaskType, ObjIDType> local_objects;
  std::unordered_map<vt_lb::model::SharedBlockType, double> shared_sizes;
  std::unordered_map<vt_lb::model::SharedBlockType, NodeType> shared_homes;
  std::unordered_set<vt_lb::model::SharedBlockType> shared_ids;
  bool const use_shared_blocks =
    gamma_ != 0.0 || delta_ != 0.0 || memory_threshold_ > 0.0;

  bool const has_user_data = load_model_->hasUserData();
  bool has_memory_info = false;
  bool has_working_memory = false;
  bool has_footprint_memory = false;
  bool has_serialized_memory = false;
  bool has_shared_memory = false;
  double rank_footprint = 0.0;

  for (auto it = load_model_->begin(); it != load_model_->end(); ++it) {
    auto const obj = *it;
    auto const task_id = static_cast<ExternalTaskType>(obj.id);
    local_objects.emplace(task_id, obj);

    double working = 0.0;
    double footprint = 0.0;
    double serialized = 0.0;
    std::optional<double> shared_size;
    std::optional<NodeType> shared_home;
    std::optional<vt_lb::model::SharedBlockType> shared_id;

    if (has_user_data) {
      auto const user_data = load_model_->getUserData(obj, when);
      for (auto const& [key, value] : user_data) {
        auto const numeric = numericValue(value);
        if (!numeric) {
          continue;
        }
        if (key == "task_working_bytes") {
          working = *numeric;
          has_working_memory = true;
        } else if (key == "task_footprint_bytes") {
          footprint = *numeric;
          has_footprint_memory = true;
        } else if (key == "task_serialized_bytes") {
          serialized = *numeric;
          has_serialized_memory = true;
        } else if (key == "rank_working_bytes") {
          rank_footprint = *numeric;
        } else if (key == "shared_id") {
          shared_id = static_cast<vt_lb::model::SharedBlockType>(*numeric);
        } else if (key == "shared_bytes") {
          shared_size = *numeric;
        } else if (key == "home_rank") {
          shared_home = static_cast<NodeType>(*numeric);
        }
      }
    }

    vt_lb::model::Task task{
      task_id,
      obj.getHomeNode(),
      obj.getCurrNode(),
      obj.isMigratable(),
      vt_lb::model::TaskMemory{working, footprint, serialized},
      load_model_->getModeledLoad(obj, when)
    };
    if (shared_id && use_shared_blocks) {
      task.addSharedBlock(*shared_id);
      shared_ids.insert(*shared_id);
      has_shared_memory = true;
      if (shared_size) {
        shared_sizes[*shared_id] = *shared_size;
      }
      if (shared_home) {
        shared_homes[*shared_id] = *shared_home;
      }
    }
    phase_data->addTask(task);
  }

  for (auto const& [key, volume] : *comm_data) {
    if (key.selfEdge()) {
      continue;
    }
    if (key.commCategory() == elm::CommCategory::SendRecv) {
      auto const from = key.fromObj();
      auto const to = key.toObj();
      vt_lb::model::Edge edge{
        static_cast<ExternalTaskType>(from.id),
        static_cast<ExternalTaskType>(to.id),
        volume.bytes,
        from.getCurrNode(),
        to.getCurrNode()
      };
      edge.setNumMessages(static_cast<int>(volume.messages));
      phase_data->addCommunication(edge);
    } else if (
      use_shared_blocks && (
        key.commCategory() == elm::CommCategory::WriteShared ||
        key.commCategory() == elm::CommCategory::ReadOnlyShared
      )
    ) {
      auto const shared_id = static_cast<vt_lb::model::SharedBlockType>(key.sharedID());
      shared_ids.insert(shared_id);
      shared_homes[shared_id] = key.toNode();
      shared_sizes[shared_id] = volume.bytes;
      has_shared_memory = true;
    }
  }

  for (auto const shared_id : shared_ids) {
    auto const home = shared_homes.count(shared_id) != 0
      ? shared_homes.at(shared_id)
      : this_node;
    auto const size = shared_sizes.count(shared_id) != 0
      ? shared_sizes.at(shared_id)
      : 0.0;
    phase_data->addSharedBlock({shared_id, size, home});
  }
  phase_data->setRankFootprintBytes(rank_footprint);
  phase_data->setRankMaxMemoryAvailable(memory_threshold_);

  has_memory_info = memory_threshold_ > 0.0 && (
    has_working_memory || has_footprint_memory || has_serialized_memory ||
    has_shared_memory || rank_footprint > 0.0
  );

  ExternalConfig external_config{static_cast<int>(theContext()->getNumNodes())};
  external_config.f_ = fanout_;
  external_config.k_max_ = rounds_;
  external_config.num_iters_ = num_iters_;
  external_config.num_trials_ = num_trials_;
  external_config.seed_ = seed_;
  external_config.deterministic_ = deterministic_;
  external_config.converge_tolerance_ = converge_tolerance_;
  external_config.work_model_.rank_alpha = alpha_;
  external_config.work_model_.beta = beta_;
  external_config.work_model_.gamma = gamma_;
  external_config.work_model_.delta = delta_;
  external_config.work_model_.has_memory_info = has_memory_info;
  external_config.work_model_.has_task_working_memory_info = has_working_memory;
  external_config.work_model_.has_task_footprint_memory_info = has_footprint_memory;
  external_config.work_model_.has_task_serialized_memory_info = has_serialized_memory;
  external_config.work_model_.has_shared_block_memory_info = has_shared_memory;
  external_config.cluster_based_on_communication_ = beta_ != 0.0 || gamma_ != 0.0;
  external_config.cluster_based_on_shared_blocks_ = delta_ != 0.0;

  using Criterion = vt_lb::algo::temperedlb::CriterionEnum;
  if (criterion_ == "Grapevine") {
    external_config.criterion_ = Criterion::Grapevine;
  } else if (criterion_ == "ModifiedGrapevine") {
    external_config.criterion_ = Criterion::ModifiedGrapevine;
  } else {
    vtAbort("Unknown TemperedLB criterion: " + criterion_);
  }

  using ObjectOrder = ExternalTransferUtil::ObjectOrder;
  static std::unordered_map<std::string, ObjectOrder> const orderings = {
    {"Arbitrary", ObjectOrder::Arbitrary},
    {"ElmID", ObjectOrder::ElmID},
    {"FewestMigrations", ObjectOrder::FewestMigrations},
    {"SmallObjects", ObjectOrder::SmallObjects},
    {"LargestObjects", ObjectOrder::LargestObjects}
  };
  auto const ordering = orderings.find(ordering_);
  vtAbortIf(ordering == orderings.end(), "Unknown TemperedLB ordering: " + ordering_);
  external_config.obj_ordering_ = ordering->second;

  using CMFType = ExternalTransferUtil::CMFType;
  static std::unordered_map<std::string, CMFType> const cmfs = {
    {"Original", CMFType::Original},
    {"NormByMax", CMFType::NormByMax},
    {"NormByMaxExcludeIneligible", CMFType::NormByMaxExcludeIneligible}
  };
  auto const cmf = cmfs.find(cmf_);
  vtAbortIf(cmf == cmfs.end(), "Unknown TemperedLB cmf: " + cmf_);
  external_config.cmf_type_ = cmf->second;

  comm::CommVT communicator;
  ExternalLB lb{communicator, external_config};
  lb.inputData(std::move(phase_data));
  auto const local_after = lb.run();

  Mapping local_mapping;
  local_mapping.tasks.reserve(local_after.size());
  for (auto const task : local_after) {
    local_mapping.tasks.emplace_back(task, this_node);
  }
  global_mapping_.tasks.clear();
  runInEpochCollective("TemperedLB: gather external mapping", [&] {
    proxy_.allreduce<&TemperedLB::collectMapping, collective::PlusOp>(
      std::move(local_mapping)
    );
  });

  std::unordered_map<ExternalTaskType, NodeType> destinations;
  for (auto const& [task, rank] : global_mapping_.tasks) {
    destinations.emplace(task, rank);
  }

  for (auto const& [task_id, object] : local_objects) {
    auto const destination = destinations.find(task_id);
    vtAssert(destination != destinations.end(), "LB result omitted a VT object");
    if (object.isMigratable() && destination->second != object.getCurrNode()) {
      migrateObjectTo(object, destination->second);
    }
  }
#else
  vtAbort("TemperedLB is unavailable because VT load balancing is disabled");
#endif
}

}}}} /* end namespace vt::vrt::collection::lb */
