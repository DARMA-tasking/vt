/*
//@HEADER
// *****************************************************************************
//
//                                temperedlb.cc
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
#include "vt/timing/timing.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/vrt/collection/balance/temperedlb/temperedlb.h"
#include "vt/vrt/collection/balance/temperedlb/tempered_msgs.h"
#include "vt/vrt/collection/balance/temperedlb/tempered_constants.h"
#include "vt/vrt/collection/balance/temperedlb/criterion.h"
#include "vt/vrt/collection/balance/lb_args_enum_converter.h"
#include "vt/context/context.h"

#include <cstdint>
#include <random>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <limits>

namespace vt { namespace vrt { namespace collection { namespace lb {

void TemperedLB::init(objgroup::proxy::Proxy<TemperedLB> in_proxy) {
  proxy_ = in_proxy;
  auto const this_node = theContext()->getNode();
  gen_propagate_.seed(this_node + 12345);
  gen_sample_.seed(this_node + 54321);
}

bool TemperedLB::isUnderloaded(LoadType load) const {
  return load < target_max_load_ * temperedlb_load_threshold;
}

bool TemperedLB::isOverloaded(LoadType load) const {
  return load > target_max_load_ * temperedlb_load_threshold;
}

/*static*/ std::unordered_map<std::string, std::string>
TemperedLB::getInputKeysWithHelp() {
  std::unordered_map<std::string, std::string> const keys_help = {
    {
      "knowledge",
      R"(
Values: {UserDefined, Complete, Log}
Default: Log
Description:
  How the fanout and the number of rounds are determined. Options are:
    UserDefined: the fanout and rounds must be set explicitly.
    Complete: the fanout will be as large as possible, with only one round, to
      guarantee full information.
    Log: choose rounds and/or fanout based on log rule. Either fanout or
      rounds must be explicitly set, but not both.  The relationship between
      rounds and fanout will be approximately
      rounds = log(num_ranks)/log(fanout).
)"
    },
    {
      "fanout",
      R"(
Values: <uint16_t>
Default: N/A
Description:
  The number of ranks each underloaded rank will communicate with. May be
  determined automatically by an appropriate choice for knowledge.
)"
    },
    {
      "rounds",
      R"(
Values: <uint8_t>
Default: N/A
Description:
  The number of information propagation rounds. May be determined automatically
  by an appropriate choice for knowledge.
)"
    },
    {
      "iters",
      R"(
Values: <uint16_t>
Default: 4
Description:
  The number of iterations of the information propagation and transfer steps.
)"
    },
    {
      "trials",
      R"(
Values: <uint16_t>
Default: 1
Description:
  How many times to repeat the requested number of iterations, hoping to find
  a better imbalance. Increasing this helps if it's easy to get stuck in a
  local minimum.
)"
    },
    {
      "criterion",
      R"(
Values: {Grapevine, ModifiedGrapevine}
Default: ModifiedGrapevine
Description:
  The criterion used for evaluating if a proposed transfer should be accepted.
  Options are:
    Grapevine: accept if the proposed transfer will not overload the recipient.
    ModifiedGrapevine: accept if the proposed transfer will not make the load of
      the recipient higher than was the load of the sender immediately before
      proposing the transfer.
)"
    },
    {
      "inform",
      R"(
Values: {SyncInform, AsyncInform}
Default: AsyncInform
Description:
  Approach used to track rounds in the information propagation step. Options
  are:
    SyncInform: synchronous sharing of underloaded processor loads. The round
      number is defined at the processor level. This approach propagates known
      loads after all messages for a round are received, maximizing the amount
      of information propagated per round, but has a synchronization cost.
    AsyncInform: asynchronous sharing of underloaded processor loads. The round
      number is defined at the message level. This approach propagates known
      loads when the first message for a round is received, avoiding the
      synchronization cost but delaying the propagation of some information
      until the following round.
)"
    },
    {
      "transfer",
      R"(
Values: {Original, Recursive, SwapClusters}
Default: Original
Description:
  Transfer strategy to be used in transfer stage. Options are:
    Original: transfer one object per transfer as in original Grapevine approach.
    Recursive: original strategy improved by recursion.
      When single object transfer is rejected, attempt to recurse in order to
      pull more objects into the transfer and hereby minimize work added by
      said transfer.
      This is especially useful when communication is taken into account, as
      object transfers typically disrupt local vs. global communication edges.
    SwapClusters: form object clusters and attempt to perform swaps.
      Object can be clustered according to arbitrary definition, and swaps
      of entire clusters, according the nullset, between ranks are attempted.
      This is especially useful when shared memory constraints are present,
      as breaking shared memory clusters results in higher overall memory
      footprint, in constrast with whole cluster swaps.
)"
    },
    {
      "ordering",
      R"(
Values: {Arbitrary, ElmID, FewestMigrations, SmallObject, LargestObjects}
Default: FewestMigrations
Description:
  The order in which local objects are considered for transfer. Options are:
    Arbitrary: iterate as defined by the unordered_map.
    ElmID: sort ascending by the element ID.
    FewestMigrations: order for fewest migrations. Start with the object with
      the smallest load that can be transferred to drop the processor load
      below the average, then order by descending load for objects with smaller
      loads, and finally order by ascending load for objects with larger loads.
    SmallObjects: order for migrating the objects with the smallest loads.
      Find the object with the smallest load where the sum of its own load and
      all smaller loads meets or exceeds the amount by which the load of this
      processor load exceeds the target load. Order starting with that object,
      then by descending load for objects with smaller loads, and finally by
      ascending load for objects with larger loads.
    LargestObjects: order by descending load.
)"
    },
    {
      "cmf",
      R"(
Values: {Original, NormByMax, NormBySelf, NormByMaxExcludeIneligible}
Default: NormByMax
Description:
  Approach for computing the CMF used to pick an object to transfer. Options
  are:
    Original: the original formula but re-computed after each accepted transfer.
      Remove processors from the CMF as soon as they exceed the target (e.g.,
      processor-average) load. Use a CMF factor of 1.0/x, where x is the target
      load.
    NormByMax: compute the CMF factor using the largest processor load in the
      CMF. Do not remove processors from the CMF that exceed the target load
      until the next iteration. Use a CMF factor of 1.0/x, where x is the
      greater of the target load and the load of the most loaded processor in
      the CMF.
    NormBySelf: compute the CMF factor using the load of this processor. Do not
      remove processors from the CMF that exceed the target load until the next
      iteration. Use a CMF factor of 1.0/x, where x is the load of the processor
      that is computing the CMF.
    NormByMaxExcludeIneligible: narrow the CMF to only include processors that
      can accommodate the transfer. Use a CMF factor of 1.0/x, where x is the
      greater of the target load and the load of the most loaded processor in
      the CMF. Only include processors in the CMF that will pass the chosen
      Criterion for the object being considered for transfer.
)"
    },
    {
      "deterministic",
      R"(
Values: {true, false}
Default: false
Description:
  Whether to make migration choices deterministic. This will only lead to
  reproducibility when paired with deterministic object loads, for example when
  using a driver that feeds the load balancer object loads read from LB data
  files.  Enabling this requires choosing options for inform and ordering that
  are themselves deterministic.
)"
    },
    {
      "rollback",
      R"(
Values: {true, false}
Default: true
Description:
  If the final iteration of a trial has a worse imbalance than any earlier
  iteration, it will roll back to the iteration with the best imbalance.
)"
    },
    {
      "targetpole",
      R"(
Values: {true, false}
Default: false
Description:
  When an object load exceeds the processor-average load (i.e., we have a "long
  pole"), adjust the target load to be the maximum object load ("longest pole")
  instead of the processor-average load.
)"
    },
    {
      "memory_threshold",
      R"(
Values: <double>
Defaut: 0
Description: The memory threshold TemperedLB should strictly stay under which is
respected if memory information is present in the user-defined data.
)"
    }
  };
  return keys_help;
}

void TemperedLB::inputParams(balance::ConfigEntry* config) {
  auto keys_help = getInputKeysWithHelp();

  std::vector<std::string> allowed;
  for (auto&& elm : keys_help) {
    allowed.push_back(elm.first);
  }
  config->checkAllowedKeys(allowed);

  // the following options interact with each other, so we need to know
  // which were defaulted and which were explicitly specified
  auto params = config->getParams();
  bool specified_knowledge = params.find("knowledge") != params.end();
  bool specified_fanout    = params.find("fanout")    != params.end();
  bool specified_rounds    = params.find("rounds")    != params.end();

  balance::LBArgsEnumConverter<KnowledgeEnum> knowledge_converter_(
    "knowledge", "KnowledgeEnum", {
      {KnowledgeEnum::UserDefined, "UserDefined"},
      {KnowledgeEnum::Complete,    "Complete"},
      {KnowledgeEnum::Log,         "Log"}
    }
  );
  knowledge_ = knowledge_converter_.getFromConfig(config, knowledge_);

  vtAbortIf(
    specified_knowledge && knowledge_ == KnowledgeEnum::Log &&
    specified_fanout && specified_rounds,
    "TemperedLB: You must leave fanout and/or rounds unspecified when "
    "knowledge=Log"
  );
  vtAbortIf(
    !specified_knowledge && knowledge_ == KnowledgeEnum::Log &&
    specified_fanout && specified_rounds,
    "TemperedLB: You must use knowledge=UserDefined if you want to explicitly "
    "set both fanout and rounds"
  );
  vtAbortIf(
    knowledge_ == KnowledgeEnum::Complete &&
    (specified_fanout || specified_rounds),
    "TemperedLB: You must leave fanout and rounds unspecified when "
    "knowledge=Complete"
  );
  vtAbortIf(
    knowledge_ == KnowledgeEnum::UserDefined &&
    (!specified_fanout || !specified_rounds),
    "TemperedLB: You must explicitly set both fanout and rounds when "
    "knowledge=UserDefined"
  );

  auto num_nodes = theContext()->getNumNodes();
  if (knowledge_ == KnowledgeEnum::Log) {
    if (specified_fanout) {
      // set the rounds based on the chosen fanout: k=log_f(p)
      f_ = config->getOrDefault<int32_t>("fanout", f_);
      k_max_ = static_cast<uint8_t>(
        std::ceil(std::log(num_nodes)/std::log(f_))
      );
    } else if (specified_rounds) {
      // set the fanout based on the chosen rounds: f=p^(1/k)
      k_max_ = config->getOrDefault<int32_t>("rounds", k_max_);
      f_ = static_cast<uint16_t>(std::ceil(std::pow(num_nodes, 1.0/k_max_)));
    } else {
      // set both the fanout and the rounds
      k_max_ = static_cast<uint16_t>(std::max(1.0,
        std::round(std::sqrt(std::log(num_nodes)/std::log(2.0)))
      ));
      f_ = static_cast<uint16_t>(std::ceil(std::pow(num_nodes, 1.0/k_max_)));
    }
  } else if (knowledge_ == KnowledgeEnum::Complete) {
    f_ = num_nodes - 1;
    k_max_ = 1;
  } else { // knowledge_ == KnowledgeEnum::UserDefined
    // if either of these was omitted, a default will be used, which probably
    // isn't desirable
    f_     = config->getOrDefault<int32_t>("fanout", f_);
    k_max_ = config->getOrDefault<int32_t>("rounds", k_max_);
  }

  if (f_ < 1) {
    auto s = fmt::format(
      "TemperedLB: fanout={} is invalid; fanout must be positive",
      f_
    );
    vtAbort(s);
  }
  if (k_max_ < 1) {
    auto s = fmt::format(
      "TemperedLB: rounds={} is invalid; rounds must be positive",
      k_max_
    );
    vtAbort(s);
  }

  num_iters_     = config->getOrDefault<int32_t>("iters", num_iters_);
  num_trials_    = config->getOrDefault<int32_t>("trials", num_trials_);

  deterministic_ = config->getOrDefault<bool>("deterministic", deterministic_);
  rollback_      = config->getOrDefault<bool>("rollback", rollback_);
  target_pole_   = config->getOrDefault<bool>("targetpole", target_pole_);
  mem_thresh_    = config->getOrDefault<double>("memory_threshold", mem_thresh_);

  balance::LBArgsEnumConverter<CriterionEnum> criterion_converter_(
    "criterion", "CriterionEnum", {
      {CriterionEnum::Grapevine,         "Grapevine"},
      {CriterionEnum::ModifiedGrapevine, "ModifiedGrapevine"}
    }
  );
  criterion_ = criterion_converter_.getFromConfig(config, criterion_);

  balance::LBArgsEnumConverter<InformTypeEnum> inform_type_converter_(
    "inform", "InformTypeEnum", {
      {InformTypeEnum::SyncInform,  "SyncInform"},
      {InformTypeEnum::AsyncInform, "AsyncInform"}
    }
  );
  inform_type_ = inform_type_converter_.getFromConfig(config, inform_type_);

  balance::LBArgsEnumConverter<TransferTypeEnum> transfer_type_converter_(
    "transfer", "TransferTypeEnum", {
      {TransferTypeEnum::Original,        "Original"},
      {TransferTypeEnum::Recursive,       "Recursive"},
      {TransferTypeEnum::SwapClusters,    "SwapClusters"}
    }
  );
  transfer_type_ = transfer_type_converter_.getFromConfig(config, transfer_type_);

  balance::LBArgsEnumConverter<ObjectOrderEnum> obj_ordering_converter_(
    "ordering", "ObjectOrderEnum", {
      {ObjectOrderEnum::Arbitrary,        "Arbitrary"},
      {ObjectOrderEnum::ElmID,            "ElmID"},
      {ObjectOrderEnum::FewestMigrations, "FewestMigrations"},
      {ObjectOrderEnum::SmallObjects,     "SmallObjects"},
      {ObjectOrderEnum::LargestObjects,   "LargestObjects"}
    }
  );
  obj_ordering_ = obj_ordering_converter_.getFromConfig(config, obj_ordering_);

  balance::LBArgsEnumConverter<CMFTypeEnum> cmf_type_converter_(
    "cmf", "CMFTypeEnum", {
      {CMFTypeEnum::Original,                   "Original"},
      {CMFTypeEnum::NormByMax,                  "NormByMax"},
      {CMFTypeEnum::NormBySelf,                 "NormBySelf"},
      {CMFTypeEnum::NormByMaxExcludeIneligible, "NormByMaxExcludeIneligible"}
    }
  );
  cmf_type_ = cmf_type_converter_.getFromConfig(config, cmf_type_);

  vtAbortIf(
    inform_type_ == InformTypeEnum::AsyncInform && deterministic_,
    "Asynchronous informs allow race conditions and thus are not "
    "deterministic; use inform=SyncInform"
  );
  vtAbortIf(
    obj_ordering_ == ObjectOrderEnum::Arbitrary && deterministic_,
    "Arbitrary object ordering is not deterministic; use ordering=ElmID "
    "or another option"
  );

  if (theContext()->getNode() == 0) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::inputParams: using knowledge={}, fanout={}, rounds={}, "
      "iters={}, criterion={}, trials={}, deterministic={}, inform={}, "
      "ordering={}, cmf={}, rollback={}, targetpole={}\n",
      knowledge_converter_.getString(knowledge_), f_, k_max_, num_iters_,
      criterion_converter_.getString(criterion_), num_trials_, deterministic_,
      inform_type_converter_.getString(inform_type_),
      transfer_type_converter_.getString(transfer_type_),
      obj_ordering_converter_.getString(obj_ordering_),
      cmf_type_converter_.getString(cmf_type_), rollback_, target_pole_
    );
  }
}

void TemperedLB::runLB(LoadType total_load) {
  bool should_lb = false;

  // Compute load statistics
  this_load = total_load;
  stats = *getStats();
  auto const avg  = stats.at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::avg
  );
  auto const max  = stats.at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::max
  );
  auto const pole = stats.at(lb::Statistic::Object_load_modeled).at(
    lb::StatisticQuantity::max
  );
  auto const imb  = stats.at(lb::Statistic::Rank_load_modeled).at(
    lb::StatisticQuantity::imb
  );
  auto const load = this_load;

  if (target_pole_) {
    // we can't get the processor max lower than the max object load, so
    // modify the algorithm to define overloaded as exceeding the max
    // object load instead of the processor average load
    target_max_load_ = (pole > avg ? pole : avg);
  } else {
    target_max_load_ = avg;
  }

  // Use an absolute minimal bound on average load to load-balance
  if (avg > 1e-10) {
    should_lb = max > (run_temperedlb_tolerance + 1.0) * target_max_load_;
  }

  // Report statistics from head rank
  if (theContext()->getNode() == 0) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::runLB: avg={}, max={}, pole={}, imb={}, load={}, should_lb={}\n",
      LoadType(avg), LoadType(max), LoadType(pole), imb,
      LoadType(load), should_lb
    );

    if (!should_lb) {
      vt_print(
        lb,
        "TemperedLB decided to skip rebalancing due to low imbalance\n"
      );
    }
  }

  // Perform load rebalancing when deemed necessary
  if (should_lb) {
    doLBStages(imb);
  }
}

void TemperedLB::readClustersMemoryData() {
 if (load_model_->hasUserData()) {
    for (auto obj : *load_model_) {
      if (obj.isMigratable()) {
        auto data_map = load_model_->getUserData(
          obj, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}
        );

        SharedIDType shared_id = -1;
        BytesType shared_bytes = 0;
        BytesType working_bytes = 0;
        for (auto const& [key, variant] : data_map) {
          if (key == "shared_id") {
            // Because of how JSON is stored this is always a double, even
            // though it should be an integer
            if (double const* val = std::get_if<double>(&variant)) {
              shared_id = static_cast<int>(*val);
            } else {
              vtAbort("\"shared_id\" in variant does not match double");
            }
          }
          if (key == "shared_bytes") {
            if (BytesType const* val = std::get_if<BytesType>(&variant)) {
              shared_bytes = *val;
            } else {
              vtAbort("\"shared_bytes\" in variant does not match double");
            }
          }
          if (key == "task_working_bytes") {
            if (BytesType const* val = std::get_if<BytesType>(&variant)) {
              working_bytes = *val;
            } else {
              vtAbort("\"working_bytes\" in variant does not match double");
            }
          }
          if (key == "rank_working_bytes") {
            if (BytesType const* val = std::get_if<BytesType>(&variant)) {
              rank_bytes_ = *val;
            } else {
              vtAbort("\"rank_bytes\" in variant does not match double");
            }
          }
          // @todo: for now, skip "task_serialized_bytes" and
          // "task_footprint_bytes"
        }

        vt_debug_print(
          verbose, temperedlb,
          "obj={} shared_block={} bytes={}\n",
          obj, shared_id, shared_bytes
        );

        obj_shared_block_[obj] = shared_id;
        obj_working_bytes_[obj] = working_bytes;
        shared_block_size_[shared_id] = shared_bytes;
      }
    }
  }
}

void TemperedLB::computeClusterSummary() {
  cur_clusters_.clear();
  for (auto const& [shared_id, shared_bytes] : shared_block_size_) {
    LoadType cluster_load = 0;
    for (auto const& [obj_id, obj_load] : cur_objs_) {
      if (auto iter = obj_shared_block_.find(obj_id); iter != obj_shared_block_.end()) {
        if (iter->second == shared_id) {
          cluster_load += obj_load;
        }
      }
    }
    if (cluster_load != 0) {
      cur_clusters_[shared_id] = std::make_tuple(shared_bytes, cluster_load);
    }
  }
}

BytesType TemperedLB::computeMemoryUsage() {
  // Compute bytes used by shared blocks mapped here based on object mapping
  auto const blocks_here = getSharedBlocksHere();

  double total_shared_bytes = 0;
  for (auto const& block_id : blocks_here) {
    total_shared_bytes += shared_block_size_.find(block_id)->second;
  }

  // Compute max object size
  // @todo: Slight issue here that this will only count migratable objects
  // (those contained in cur_objs), for our current use case this is not a
  // problem, but it should include the max of non-migratable
  double max_object_working_bytes = 0;
  for (auto const& [obj_id, _] : cur_objs_)  {
    if (obj_working_bytes_.find(obj_id) != obj_working_bytes_.end()) {
      max_object_working_bytes_ =
        std::max(max_object_working_bytes, obj_working_bytes_.find(obj_id)->second);
    } else {
      vt_debug_print(
        verbose, temperedlb,
        "Warning: working bytes not found for object: {}\n", obj_id
      );
    }
  }
  return current_memory_usage_ =
    rank_bytes_ + total_shared_bytes + max_object_working_bytes_;
}

std::set<SharedIDType> TemperedLB::getSharedBlocksHere() const {
  std::set<SharedIDType> blocks_here;
  for (auto const& [obj, _] : cur_objs_) {
    if (obj_shared_block_.find(obj) != obj_shared_block_.end()) {
      blocks_here.insert(obj_shared_block_.find(obj)->second);
    }
  }
  return blocks_here;
}

void TemperedLB::doLBStages(LoadType start_imb) {
  decltype(this->cur_objs_) best_objs;
  LoadType best_load = 0;
  LoadType best_imb = start_imb + 10;
  uint16_t best_trial = 0;

  auto this_node = theContext()->getNode();

  // Read in memory information if it's available before be do any trials
  readClustersMemoryData();

  if (transfer_type_ == TransferTypeEnum::SwapClusters) {
    has_memory_data_ = true;
  }

  for (trial_ = 0; trial_ < num_trials_; ++trial_) {
    // Clear out data structures
    selected_.clear();
    underloaded_.clear();
    load_info_.clear();
    other_rank_clusters_.clear();
    max_load_over_iters_.clear();
    is_overloaded_ = is_underloaded_ = false;
    is_subclustering_ = false;
    ready_to_satisfy_locks_ = false;

    LoadType best_imb_this_trial = start_imb + 10;

    for (iter_ = 0; iter_ < num_iters_; iter_++) {
      bool first_iter = iter_ == 0;

      if (first_iter) {
        // Copy this node's object assignments to a local, mutable copy
        cur_objs_.clear();
        for (auto obj : *load_model_) {
          if (obj.isMigratable()) {
            cur_objs_[obj] = getModeledValue(obj);
          }
        }
        this_new_load_ = this_load;
      } else {
        // Clear out data structures from previous iteration
        selected_.clear();
        underloaded_.clear();
        load_info_.clear();
        is_overloaded_ = is_underloaded_ = false;
        is_subclustering_ = false;
        ready_to_satisfy_locks_ = false;
        other_rank_clusters_.clear();

        // Not clearing shared_block_size_ because this never changes and
        // the knowledge might be useful
      }

      vt_debug_print(
        normal, temperedlb,
        "TemperedLB::doLBStages: (before) running trial={}, iter={}, "
        "num_iters={}, load={}, new_load={}\n",
        trial_, iter_, num_iters_, LoadType(this_load),
        LoadType(this_new_load_)
      );

      if (has_memory_data_) {
        vt_debug_print(
          terse, temperedlb,
          "Current memory info: total memory usage={}, shared blocks here={}, "
          "memory_threshold={}\n", computeMemoryUsage(),
          getSharedBlocksHere().size(), mem_thresh_
        );

        computeClusterSummary();

        // Verbose printing about local clusters
        for (auto const& [shared_id, value] : cur_clusters_) {
          auto const& [shared_bytes, cluster_load] = value;
          vt_debug_print(
            verbose, temperedlb,
            "Local cluster: id={}, bytes={}, load={}\n",
            shared_id, shared_bytes, cluster_load
          );
        }
      }

      if (isOverloaded(this_new_load_)) {
        is_overloaded_ = true;
      } else if (isUnderloaded(this_new_load_)) {
        is_underloaded_ = true;
      }

      // Perform requested type of information stage
      switch (inform_type_) {
      case InformTypeEnum::SyncInform:
        informSync();
        break;
      case InformTypeEnum::AsyncInform:
        informAsync();
        break;
      default:
        vtAbort("TemperedLB:: Unsupported inform type");
      }

      // Some very verbose printing about all remote clusters we know about that
      // we can shut off later
      for (auto const& [node, clusters] : other_rank_clusters_) {
        for (auto const& [shared_id, value] : clusters) {
          auto const& [shared_bytes, cluster_load] = value;
          vt_debug_print(
            verbose, temperedlb,
            "Remote cluster: node={}, id={}, bytes={}, load={}\n",
            node, shared_id, shared_bytes, cluster_load
          );
        }
      }

      // Move remove cluster information to shared_block_size_ so we have all
      // the sizes in the same place
      for (auto const& [node, clusters] : other_rank_clusters_) {
        for (auto const& [shared_id, value] : clusters) {
          auto const& [shared_bytes, _] = value;
          shared_block_size_[shared_id] = shared_bytes;
        }
      }

      // Execute transfer stage
      switch (transfer_type_) {
      case TransferTypeEnum::Original:
        originalTransfer();
        break;
      case TransferTypeEnum::Recursive:
	vtAbort("TemperedLB:: Unimplemented transfer type: Recursive");
        break;
      case TransferTypeEnum::SwapClusters:
	swapClusters();
        break;
      default:
        vtAbort("TemperedLB:: Unsupported transfer type");
      }

      vt_debug_print(
        verbose, temperedlb,
        "TemperedLB::doLBStages: (after) running trial={}, iter={}, "
        "num_iters={}, load={}, new_load={}\n",
        trial_, iter_, num_iters_, LoadType(this_load),
        LoadType(this_new_load_)
      );

      if (
        rollback_ ||
        theConfig()->vt_debug_temperedlb ||
        (iter_ == num_iters_ - 1) ||
        transfer_type_ == TransferTypeEnum::SwapClusters
      ) {
        runInEpochCollective("TemperedLB::doLBStages -> Rank_load_modeled", [=] {
          // Perform the reduction for Rank_load_modeled -> processor load only
          proxy_.allreduce<&TemperedLB::loadStatsHandler, collective::PlusOp>(
            std::vector<balance::LoadData>{
              {balance::LoadData{Statistic::Rank_load_modeled, this_new_load_}}
            }
          );
        });
      }

      if (rollback_ || (iter_ == num_iters_ - 1)) {
        // if known, save the best iteration within any trial so we can roll back
        if (new_imbalance_ < best_imb && new_imbalance_ <= start_imb) {
          best_load = this_new_load_;
          best_objs = cur_objs_;
          best_imb = new_imbalance_;
          best_trial = trial_;
        }
        if (new_imbalance_ < best_imb_this_trial) {
          best_imb_this_trial = new_imbalance_;
        }
      }
    }

    if (this_node == 0) {
      vt_debug_print(
        terse, temperedlb,
        "TemperedLB::doLBStages: trial={} {} imb={:0.4f}\n",
        trial_, rollback_ ? "best" : "final", best_imb_this_trial
      );
    }

    // Clear out for next try or for not migrating by default
    cur_objs_.clear();
    this_new_load_ = this_load;
  }

  if (best_imb <= start_imb) {
    // load the configuration with the best imbalance
    cur_objs_ = best_objs;
    this_load = this_new_load_ = best_load;
    new_imbalance_ = best_imb;

    if (this_node == 0) {
      vt_debug_print(
        terse, temperedlb,
        "TemperedLB::doLBStages: chose trial={} with imb={:0.4f}\n",
        best_trial, new_imbalance_
      );
    }
  } else if (this_node == 0) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::doLBStages: rejected all trials because they would increase imbalance\n"
    );
  }

  // Concretize lazy migrations by invoking the BaseLB object migration on new
  // object node assignments
  thunkMigrations();
}

void TemperedLB::loadStatsHandler(std::vector<balance::LoadData> const& vec) {
  auto const& in = vec[0];
  new_imbalance_ = in.I();

  max_load_over_iters_.push_back(in.max());

  auto this_node = theContext()->getNode();
  if (this_node == 0) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::loadStatsHandler: trial={} iter={} max={} min={} "
      "avg={} pole={} imb={:0.4f}\n",
      trial_, iter_, LoadType(in.max()),
      LoadType(in.min()), LoadType(in.avg()),
      LoadType(stats.at(
        lb::Statistic::Object_load_modeled
      ).at(lb::StatisticQuantity::max)),
      in.I()
    );
  }
}

void TemperedLB::rejectionStatsHandler(int n_rejected, int n_transfers) {
  double rej = static_cast<double>(n_rejected) /
    static_cast<double>(n_rejected + n_transfers) * 100.0;

  auto this_node = theContext()->getNode();
  if (this_node == 0) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::rejectionStatsHandler: n_transfers={} n_rejected={} "
      "rejection_rate={:0.1f}%\n",
      n_transfers, n_rejected, rej
    );
  }
}

void TemperedLB::informAsync() {
  propagated_k_.assign(k_max_, false);

  vt_debug_print(
    normal, temperedlb,
    "TemperedLB::informAsync: starting inform phase: trial={}, iter={}, "
    "k_max={}, is_underloaded={}, is_overloaded={}, load={}\n",
    trial_, iter_, k_max_, is_underloaded_, is_overloaded_,
    LoadType(this_new_load_)
  );

  vtAssert(k_max_ > 0, "Number of rounds (k) must be greater than zero");

  auto const this_node = theContext()->getNode();
  if (is_underloaded_) {
    underloaded_.insert(this_node);
  }

  setup_done_ = false;
  proxy_.allreduce<&TemperedLB::setupDone>();
  theSched()->runSchedulerWhile([this]{ return not setup_done_; });

  auto propagate_epoch = theTerm()->makeEpochCollective("TemperedLB: informAsync");

  // Underloaded start the round
  if (is_underloaded_) {
    uint8_t k_cur_async = 0;
    propagateRound(k_cur_async, false, propagate_epoch);
  }

  theTerm()->finishedEpoch(propagate_epoch);

  vt::runSchedulerThrough(propagate_epoch);

  if (is_overloaded_) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::informAsync: trial={}, iter={}, known underloaded={}\n",
      trial_, iter_, underloaded_.size()
    );
  }

  vt_debug_print(
    verbose, temperedlb,
    "TemperedLB::informAsync: finished inform phase: trial={}, iter={}, "
    "k_max={}\n",
    trial_, iter_, k_max_
  );
}

void TemperedLB::informSync() {
  vt_debug_print(
    normal, temperedlb,
    "TemperedLB::informSync: starting inform phase: trial={}, iter={}, "
    "k_max={}, is_underloaded={}, is_overloaded={}, load={}\n",
    trial_, iter_, k_max_, is_underloaded_, is_overloaded_, this_new_load_
  );

  vtAssert(k_max_ > 0, "Number of rounds (k) must be greater than zero");

  auto const this_node = theContext()->getNode();
  if (is_underloaded_) {
    underloaded_.insert(this_node);
  }

  auto propagate_this_round = is_underloaded_;
  propagate_next_round_ = false;
  new_underloaded_ = underloaded_;
  new_load_info_ = load_info_;

  setup_done_ = false;
  proxy_.allreduce<&TemperedLB::setupDone>();
  theSched()->runSchedulerWhile([this]{ return not setup_done_; });

  for (k_cur_ = 0; k_cur_ < k_max_; ++k_cur_) {
    auto kbarr = theCollective()->newNamedCollectiveBarrier();
    theCollective()->barrier(nullptr, kbarr);

    auto name = fmt::format("TemperedLB: informSync k_cur={}", k_cur_);
    auto propagate_epoch = theTerm()->makeEpochCollective(name);

    // Underloaded start the first round; ranks that received on some round
    // start subsequent rounds
    if (propagate_this_round) {
      propagateRound(k_cur_, true, propagate_epoch);
    }

    theTerm()->finishedEpoch(propagate_epoch);

    vt::runSchedulerThrough(propagate_epoch);

    propagate_this_round = propagate_next_round_;
    propagate_next_round_ = false;
    underloaded_ = new_underloaded_;
    load_info_ = new_load_info_;
  }

  if (is_overloaded_) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::informSync: trial={}, iter={}, known underloaded={}\n",
      trial_, iter_, underloaded_.size()
    );
  }

  vt_debug_print(
    verbose, temperedlb,
    "TemperedLB::informSync: finished inform phase: trial={}, iter={}, "
    "k_max={}, k_cur={}\n",
    trial_, iter_, k_max_, k_cur_
  );
}

void TemperedLB::setupDone() {
  setup_done_ = true;
}

void TemperedLB::propagateRound(uint8_t k_cur, bool sync, EpochType epoch) {
  vt_debug_print(
    normal, temperedlb,
    "TemperedLB::propagateRound: trial={}, iter={}, k_max={}, k_cur={}\n",
    trial_, iter_, k_max_, k_cur
  );

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  std::uniform_int_distribution<NodeType> dist(0, num_nodes - 1);

  if (!deterministic_) {
    gen_propagate_.seed(seed_());
  }

  auto& selected = selected_;
  selected = underloaded_;
  if (selected.find(this_node) == selected.end()) {
    selected.insert(this_node);
  }

  // Determine fanout factor capped by number of nodes
  auto const fanout = std::min(f_, static_cast<decltype(f_)>(num_nodes - 1));
  vt_debug_print(
    verbose, temperedlb,
    "TemperedLB::propagateRound: trial={}, iter={}, k_max={}, k_cur={}, "
    "selected.size()={}, fanout={}\n",
    trial_, iter_, k_max_, k_cur, selected.size(), fanout
  );

  // Iterate over fanout factor
  for (int i = 0; i < fanout; i++) {
    // This implies full knowledge of all processors
    if (selected.size() >= static_cast<size_t>(num_nodes)) {
      return;
    }

    // First, randomly select a node
    NodeType random_node = uninitialized_destination;

    // Keep generating until we have a unique node for this round
    do {
      random_node = dist(gen_propagate_);
    } while (
      selected.find(random_node) != selected.end()
    );
    selected.insert(random_node);

    vt_debug_print(
      verbose, temperedlb,
      "TemperedLB::propagateRound: trial={}, iter={}, k_max={}, "
      "k_cur={}, sending={}\n",
      trial_, iter_, k_max_, k_cur, random_node
    );

    // Send message with load
    if (sync) {
      // Message in synchronous mode
      auto msg = makeMessage<LoadMsgSync>(this_node, load_info_);
      if (epoch != no_epoch) {
        envelopeSetEpoch(msg->env, epoch);
      }
      msg->addNodeLoad(this_node, this_new_load_);
      if (has_memory_data_) {
        msg->addNodeClusters(this_node, cur_clusters_);
      }
      proxy_[random_node].sendMsg<
        LoadMsgSync, &TemperedLB::propagateIncomingSync
      >(msg.get());
    } else {
      // Message in asynchronous mode
      auto msg = makeMessage<LoadMsgAsync>(this_node, load_info_, k_cur);
      if (epoch != no_epoch) {
        envelopeSetEpoch(msg->env, epoch);
      }
      msg->addNodeLoad(this_node, this_new_load_);
      if (has_memory_data_) {
        msg->addNodeClusters(this_node, cur_clusters_);
      }
      proxy_[random_node].sendMsg<
        LoadMsgAsync, &TemperedLB::propagateIncomingAsync
      >(msg.get());
    }
  }
}

void TemperedLB::propagateIncomingAsync(LoadMsgAsync* msg) {
  auto const from_node = msg->getFromNode();
  auto k_cur_async = msg->getRound();

  vt_debug_print(
    normal, temperedlb,
    "TemperedLB::propagateIncomingAsync: trial={}, iter={}, k_max={}, "
    "k_cur={}, from_node={}, load info size={}\n",
    trial_, iter_, k_max_, k_cur_async, from_node, msg->getNodeLoad().size()
  );

  auto const this_node = theContext()->getNode();
  for (auto const& [node, clusters] : msg->getNodeClusterSummary()) {
    if (
      node != this_node and
      other_rank_clusters_.find(node) == other_rank_clusters_.end()
    ) {
      other_rank_clusters_[node] = clusters;
    }
  }

  for (auto&& elm : msg->getNodeLoad()) {
    if (load_info_.find(elm.first) == load_info_.end()) {
      load_info_[elm.first] = elm.second;

      if (isUnderloaded(elm.second)) {
        underloaded_.insert(elm.first);
      }
    }
  }

  if (k_cur_async == k_max_ - 1) {
    // nothing to do but wait for termination to be detected
  } else if (propagated_k_[k_cur_async]) {
    // we already propagated this round before receiving this message
  } else {
    // send out another round
    propagated_k_[k_cur_async] = true;
    propagateRound(k_cur_async + 1, false);
  }
}

void TemperedLB::propagateIncomingSync(LoadMsgSync* msg) {
  auto const from_node = msg->getFromNode();

  // we collected more info that should be propagated on the next round
  propagate_next_round_ = true;

  vt_debug_print(
    normal, temperedlb,
    "TemperedLB::propagateIncomingSync: trial={}, iter={}, k_max={}, "
    "k_cur={}, from_node={}, load info size={}\n",
    trial_, iter_, k_max_, k_cur_, from_node, msg->getNodeLoad().size()
  );

  auto const this_node = theContext()->getNode();
  for (auto const& [node, clusters] : msg->getNodeClusterSummary()) {
    if (
      node != this_node and
      other_rank_clusters_.find(node) == other_rank_clusters_.end()
    ) {
      other_rank_clusters_[node] = clusters;
    }
  }

  for (auto&& elm : msg->getNodeLoad()) {
    if (new_load_info_.find(elm.first) == new_load_info_.end()) {
      new_load_info_[elm.first] = elm.second;

      if (isUnderloaded(elm.second)) {
        new_underloaded_.insert(elm.first);
      }
    }
  }
}

std::vector<double> TemperedLB::createCMF(NodeSetType const& under) {
  // Build the CMF
  std::vector<double> cmf = {};

  if (under.size() == 1) {
    // trying to compute the cmf for only a single object can result
    // in nan for some cmf types below, so do it the easy way instead
    cmf.push_back(1.0);
    return cmf;
  }

  double sum_p = 0.0;
  double factor = 1.0;

  switch (cmf_type_) {
  case CMFTypeEnum::Original:
    factor = 1.0 / target_max_load_;
    break;
  case CMFTypeEnum::NormBySelf:
    factor = 1.0 / this_new_load_;
    break;
  case CMFTypeEnum::NormByMax:
  case CMFTypeEnum::NormByMaxExcludeIneligible:
    {
      double l_max = 0.0;
      for (auto&& pe : under) {
        auto iter = load_info_.find(pe);
        vtAssert(iter != load_info_.end(), "Node must be in load_info_");
        auto load = iter->second;
        if (load > l_max) {
          l_max = load;
        }
      }
      factor = 1.0 / (l_max > target_max_load_ ? l_max : target_max_load_);
    }
    break;
  default:
    vtAbort("This CMF type is not supported");
  }

  for (auto&& pe : under) {
    auto iter = load_info_.find(pe);
    vtAssert(iter != load_info_.end(), "Node must be in load_info_");

    auto load = iter->second;
    sum_p += 1. - factor * load;
    cmf.push_back(sum_p);
  }

  // Normalize the CMF
  for (auto& elm : cmf) {
    elm /= sum_p;
  }

  vtAssertExpr(cmf.size() == under.size());

  return cmf;
}

NodeType TemperedLB::sampleFromCMF(
  NodeSetType const& under, std::vector<double> const& cmf
) {
  // Create the distribution
  std::uniform_real_distribution<double> dist(0.0, 1.0);

  if (!deterministic_) {
    gen_sample_.seed(seed_());
  }

  NodeType selected_node = uninitialized_destination;

  // Pick from the CMF
  auto const u = dist(gen_sample_);
  std::size_t i = 0;
  for (auto&& x : cmf) {
    if (x >= u) {
      selected_node = under[i];
      break;
    }
    i++;
  }

  return selected_node;
}

std::vector<NodeType> TemperedLB::makeUnderloaded() const {
  std::vector<NodeType> under = {};
  for (auto&& elm : load_info_) {
    if (isUnderloaded(elm.second)) {
      under.push_back(elm.first);
    }
  }
  if (deterministic_) {
    std::sort(under.begin(), under.end());
  }
  return under;
}

std::vector<NodeType> TemperedLB::makeSufficientlyUnderloaded(
  LoadType load_to_accommodate
) const {
  std::vector<NodeType> sufficiently_under = {};
  for (auto&& elm : load_info_) {
    bool eval = Criterion(criterion_)(
      this_new_load_, elm.second, load_to_accommodate, target_max_load_
    );
    if (eval) {
      sufficiently_under.push_back(elm.first);
    }
  }
  if (deterministic_) {
    std::sort(sufficiently_under.begin(), sufficiently_under.end());
  }
  return sufficiently_under;
}

TemperedLB::ElementLoadType::iterator
TemperedLB::selectObject(
  LoadType size, ElementLoadType& load, std::set<ObjIDType> const& available
) {
  if (available.size() == 0) {
    return load.end();
  } else {
    auto obj_id = *available.begin();
    auto iter = load.find(obj_id);
    if (iter != load.end()) {
      return iter;
    } else {
      vtAssert(false, "Could not find object in load info");
      return load.end();
    }
  }
}

/*static*/
std::vector<TemperedLB::ObjIDType> TemperedLB::orderObjects(
  ObjectOrderEnum obj_ordering,
  std::unordered_map<ObjIDType, LoadType> cur_objs,
  LoadType this_new_load, LoadType target_max_load
) {
  // define the iteration order
  std::vector<ObjIDType> ordered_obj_ids(cur_objs.size());

  int i = 0;
  for (auto &obj : cur_objs) {
    ordered_obj_ids[i++] = obj.first;
  }

  switch (obj_ordering) {
  case ObjectOrderEnum::ElmID:
    std::sort(
      ordered_obj_ids.begin(), ordered_obj_ids.end(), std::less<ObjIDType>()
    );
    break;
  case ObjectOrderEnum::FewestMigrations:
    {
      // first find the load of the smallest single object that, if migrated
      // away, could bring this processor's load below the target load
      auto over_avg = this_new_load - target_max_load;
      // if no objects are larger than over_avg, then single_obj_load will still
      // (incorrectly) reflect the total load, which will not be a problem
      auto single_obj_load = this_new_load;
      for (auto &obj : cur_objs) {
        auto obj_load = obj.second;
        if (obj_load > over_avg && obj_load < single_obj_load) {
          single_obj_load = obj_load;
        }
      }
      // sort largest to smallest if <= single_obj_load
      // sort smallest to largest if > single_obj_load
      std::sort(
        ordered_obj_ids.begin(), ordered_obj_ids.end(),
        [&cur_objs, single_obj_load](
          const ObjIDType &left, const ObjIDType &right
        ) {
          auto left_load = cur_objs[left];
          auto right_load = cur_objs[right];
          if (left_load <= single_obj_load && right_load <= single_obj_load) {
            // we're in the sort load descending regime (first section)
            return left_load > right_load;
          }
          // else
          // EITHER
          // a) both are above the cut, and we're in the sort ascending
          //    regime (second section), so return left < right
          // OR
          // b) one is above the cut and one is at or below, and the one
          //    that is at or below the cut needs to come first, so
          //    also return left < right
          return left_load < right_load;
        }
      );
      if (cur_objs.size() > 0) {
        vt_debug_print(
          normal, temperedlb,
          "TemperedLB::decide: over_avg={}, single_obj_load={}\n",
          LoadType(over_avg),
          LoadType(cur_objs[ordered_obj_ids[0]])
        );
      }
    }
    break;
  case ObjectOrderEnum::SmallObjects:
    {
      // first find the smallest object that, if migrated away along with all
      // smaller objects, could bring this processor's load below the target
      // load
      auto over_avg = this_new_load - target_max_load;
      std::sort(
        ordered_obj_ids.begin(), ordered_obj_ids.end(),
        [&cur_objs](const ObjIDType &left, const ObjIDType &right) {
          auto left_load = cur_objs[left];
          auto right_load = cur_objs[right];
          // sort load descending
          return left_load > right_load;
        }
      );
      auto cum_obj_load = this_new_load;
      auto single_obj_load = cur_objs[ordered_obj_ids[0]];
      for (auto obj_id : ordered_obj_ids) {
        auto this_obj_load = cur_objs[obj_id];
        if (cum_obj_load - this_obj_load < over_avg) {
          single_obj_load = this_obj_load;
          break;
        } else {
          cum_obj_load -= this_obj_load;
        }
      }
      // now that we found that object, re-sort based on it
      // sort largest to smallest if <= single_obj_load
      // sort smallest to largest if > single_obj_load
      std::sort(
        ordered_obj_ids.begin(), ordered_obj_ids.end(),
        [&cur_objs, single_obj_load](
          const ObjIDType &left, const ObjIDType &right
        ) {
          auto left_load = cur_objs[left];
          auto right_load = cur_objs[right];
          if (left_load <= single_obj_load && right_load <= single_obj_load) {
            // we're in the sort load descending regime (first section)
            return left_load > right_load;
          }
          // else
          // EITHER
          // a) both are above the cut, and we're in the sort ascending
          //    regime (second section), so return left < right
          // OR
          // b) one is above the cut and one is at or below, and the one
          //    that is at or below the cut needs to come first, so
          //    also return left < right
          return left_load < right_load;
        }
      );
      if (cur_objs.size() > 0) {
        vt_debug_print(
          normal, temperedlb,
          "TemperedLB::decide: over_avg={}, marginal_obj_load={}\n",
          LoadType(over_avg),
          LoadType(cur_objs[ordered_obj_ids[0]])
        );
      }
    }
    break;
  case ObjectOrderEnum::LargestObjects:
    {
      // sort by descending load
      std::sort(
        ordered_obj_ids.begin(), ordered_obj_ids.end(),
        [&cur_objs](const ObjIDType &left, const ObjIDType &right) {
          auto left_load = cur_objs[left];
          auto right_load = cur_objs[right];
          // sort load descending
          return left_load > right_load;
        }
      );
    }
    break;
  case ObjectOrderEnum::Arbitrary:
    break;
  default:
    vtAbort("TemperedLB::orderObjects: ordering not supported");
    break;
  }

  return ordered_obj_ids;
}

void TemperedLB::originalTransfer() {
  auto lazy_epoch = theTerm()->makeEpochCollective("TemperedLB: originalTransfer");

  // Initialize transfer and rejection counters
  int n_transfers = 0, n_rejected = 0;

  // Try to migrate objects only from overloaded ranks
  if (is_overloaded_) {
    std::vector<NodeType> under = makeUnderloaded();
    std::unordered_map<NodeType, ObjsType> migrate_objs;

    if (under.size() > 0) {
      std::vector<ObjIDType> ordered_obj_ids = orderObjects(
        obj_ordering_, cur_objs_, this_new_load_, target_max_load_
      );

      // Iterate through all the objects
      for (auto iter = ordered_obj_ids.begin(); iter != ordered_obj_ids.end(); ) {
        auto obj_id = *iter;
        auto obj_load = cur_objs_[obj_id];

        if (cmf_type_ == CMFTypeEnum::Original) {
          // Rebuild the relaxed underloaded set based on updated load of this node
          under = makeUnderloaded();
          if (under.size() == 0) {
            break;
          }
        } else if (cmf_type_ == CMFTypeEnum::NormByMaxExcludeIneligible) {
          // Rebuild the underloaded set and eliminate processors that will
          // fail the Criterion for this object
          under = makeSufficientlyUnderloaded(obj_load);
          if (under.size() == 0) {
            ++n_rejected;
            iter++;
            continue;
          }
        }
        // Rebuild the CMF with the new loads taken into account
        auto cmf = createCMF(under);

        // Select a node using the CMF
        auto const selected_node = sampleFromCMF(under, cmf);

        vt_debug_print(
          verbose, temperedlb,
          "TemperedLB::originalTransfer: selected_node={}, load_info_.size()={}\n",
          selected_node, load_info_.size()
        );

	// Find load of selected node
        auto load_iter = load_info_.find(selected_node);
        vtAssert(load_iter != load_info_.end(), "Selected node not found");
        auto& selected_load = load_iter->second;

	// Evaluate criterion for proposed transfer
        bool eval = Criterion(criterion_)(
          this_new_load_, selected_load, obj_load, target_max_load_
        );
        vt_debug_print(
          verbose, temperedlb,
          "TemperedLB::originalTransfer: trial={}, iter={}, under.size()={}, "
          "selected_node={}, selected_load={:e}, obj_id={:x}, home={}, "
          "obj_load={}, target_max_load={}, this_new_load_={}, "
          "criterion={}\n",
          trial_,
          iter_,
          under.size(),
          selected_node,
          selected_load,
          obj_id.id,
          obj_id.getHomeNode(),
          LoadType(obj_load),
          LoadType(target_max_load_),
          LoadType(this_new_load_),
          eval
        );

	// Decide about proposed migration based on criterion evaluation
        if (eval) {
          ++n_transfers;
          // Transfer the object load in seconds
          // to match the object load units on the receiving end
          migrate_objs[selected_node][obj_id] = obj_load;

          this_new_load_ -= obj_load;
          selected_load += obj_load;

          iter = ordered_obj_ids.erase(iter);
          cur_objs_.erase(obj_id);
        } else {
          ++n_rejected;
          iter++;
        }

        if (not (this_new_load_ > target_max_load_)) {
          break;
        }
      }
    }

    // Send objects to nodes
    for (auto&& migration : migrate_objs) {
      auto node = migration.first;
      lazyMigrateObjsTo(lazy_epoch, node, migration.second);
    }
  } else {
    // do nothing (underloaded-based algorithm), waits to get work from
    // overloaded nodes
  }

  theTerm()->finishedEpoch(lazy_epoch);

  vt::runSchedulerThrough(lazy_epoch);

  if (theConfig()->vt_debug_temperedlb) {
    // compute rejection rate because it will be printed
    runInEpochCollective("TemperedLB::originalTransfer -> compute rejection", [=] {
      proxy_.allreduce<&TemperedLB::rejectionStatsHandler, collective::PlusOp>(
        n_rejected, n_transfers
      );
    });
  }
}

void TemperedLB::tryLock(NodeType requesting_node, double criterion_value) {
  try_locks_.emplace(requesting_node, criterion_value);

  if (ready_to_satisfy_locks_ and not is_locked_) {
    satisfyLockRequest();
  }
}

auto TemperedLB::removeClusterToSend(
  SharedIDType shared_id, std::set<ObjIDType> objs
) {
  std::unordered_map<ObjIDType, LoadType> give_objs;
  std::unordered_map<ObjIDType, SharedIDType> give_obj_shared_block;
  std::unordered_map<SharedIDType, BytesType> give_shared_blocks_size;
  std::unordered_map<ObjIDType, BytesType> give_obj_working_bytes;

  vt_debug_print(
    verbose, temperedlb,
    "removeClusterToSend: shared_id={}\n",
    shared_id
  );

  if (shared_id != -1) {
    give_shared_blocks_size[shared_id] = shared_block_size_[shared_id];
  }

  if (objs.size() == 0) {
    for (auto const& [obj_id, obj_load] : cur_objs_) {
      if (auto iter = obj_shared_block_.find(obj_id); iter != obj_shared_block_.end()) {
        if (iter->second == shared_id) {
          give_objs[obj_id] = obj_load;
          give_obj_shared_block[obj_id] = shared_id;
          if (
            auto iter2 = give_obj_working_bytes.find(obj_id);
            iter2 != give_obj_working_bytes.end()
          ) {
            give_obj_working_bytes[obj_id] = iter2->second;
          }
        }
      }
    }
  } else {
    for (auto const& obj_id : objs) {
      give_objs[obj_id] = cur_objs_.find(obj_id)->second;
      give_obj_shared_block[obj_id] = shared_id;
      if (
        auto iter2 = give_obj_working_bytes.find(obj_id);
        iter2 != give_obj_working_bytes.end()
      ) {
        give_obj_working_bytes[obj_id] = iter2->second;
      }
    }
  }

  auto const blocks_here_before = getSharedBlocksHere();

  for (auto const& [give_obj_id, give_obj_load] : give_objs) {
    auto iter = cur_objs_.find(give_obj_id);
    vtAssert(iter != cur_objs_.end(), "Object must exist");
    // remove the object!
    cur_objs_.erase(iter);
    this_new_load_ -= give_obj_load;
  }

  auto const blocks_here_after = getSharedBlocksHere();

  vt_debug_print(
    verbose, temperedlb,
    "removeClusterToSend: before count={}, after count={}\n",
    blocks_here_before.size(), blocks_here_after.size()
  );

  return std::make_tuple(
    give_objs,
    give_obj_shared_block,
    give_shared_blocks_size,
    give_obj_working_bytes
  );
}

bool TemperedLB::memoryTransferCriterion(double try_total_bytes, double src_bytes) {
  // FIXME: incomplete implementation that ignores memory regrouping
  auto const src_after_mem = this->current_memory_usage_;
  auto const try_after_mem = try_total_bytes + src_bytes;

  return not (src_after_mem > this->mem_thresh_ or try_after_mem > this->mem_thresh_);
} // bool memoryTransferCriterion

double TemperedLB::loadTransferCriterion(double before_w_src, double before_w_dst, double src_l, double dst_l) {
  // Compute maximum work of original arrangement
  auto const w_max_0 = std::max(before_w_src, before_w_dst);

  // Compute maximum work of arrangement after proposed load transfer
  auto const after_w_src = before_w_src - src_l + dst_l;
  auto const after_w_dst = before_w_dst + src_l - dst_l;
  auto const w_max_new = std::max(after_w_src, after_w_dst);

  // Return criterion value
  return w_max_0 - w_max_new;
} // double loadTransferCriterion

void TemperedLB::considerSubClustersAfterLock(MsgSharedPtr<LockedInfoMsg> msg) {
  is_swapping_ = true;

  auto criterion = [&,this](auto src_cluster, auto try_cluster) -> double {
    auto const& [src_id, src_bytes, src_load] = src_cluster;
    auto const& [try_rank, try_total_load, try_total_bytes] = try_cluster;


    // Check whether strict bounds on memory are satisfied
    if (not memoryTransferCriterion(try_total_bytes, src_bytes)) {
      return - std::numeric_limits<double>::infinity();
    }

    // Return load transfer criterion
    return loadTransferCriterion(this_new_load_, try_total_load, src_load, 0.);
  };

  auto const& try_clusters = msg->locked_clusters;
  auto const& try_rank = msg->locked_node;
  auto const& try_load = msg->locked_load;
  auto const& try_total_bytes = msg->locked_bytes;

  vt_print(
    temperedlb,
    "considerSubClustersAfterLock: try_rank={} try_load={}\n", try_rank, try_load
  );

  // get the shared blocks current residing on this rank
  auto shared_blocks_here = getSharedBlocksHere();

  // Shared IDs when added to this rank don't put it over the limit
  std::set<SharedIDType> possible_transfers;

  for (auto const& shared_id : shared_blocks_here) {
    // Allow shared blocks that don't put it over memory or already exist on
    // try_rank
    if (try_clusters.find(shared_id) == try_clusters.end()) {
      if (try_total_bytes + shared_block_size_[shared_id] < mem_thresh_) {
        possible_transfers.insert(shared_id);
      }
    } else {
      possible_transfers.insert(shared_id);
    }
  }

  vt_print(
    temperedlb,
    "considerSubClustersAfterLock: possible_transfers={}\n",
    possible_transfers.size()
  );

  // Now, we will greedily try to find a combo of objects that will reduce our
  // max

  // We can prune some clusters out of this mix based on the requirements that
  // this is beneficial
  auto const amount_over_average = this_new_load_ - target_max_load_;
  auto const amount_under_average = target_max_load_ - try_load;

  // Any sub-cluster that is smaller than amount_over_average or smaller than
  // amount_under_average we can just skip. We start by skipping all entire
  // clusters that don't fit this criteria since sub-clusters will also be
  // eliminated from those

  vt_print(
    temperedlb,
    "considerSubClustersAfterLock: over={}, under={}\n", amount_over_average,
    amount_under_average
  );

  std::set<SharedIDType> clusters_to_split;

  for (auto const& [src_shared_id, src_cluster] : cur_clusters_) {
    auto const& [src_cluster_bytes, src_cluster_load] = src_cluster;
    if (
      src_cluster_load < amount_over_average or
      src_cluster_load < amount_under_average
    ) {
      // skip it
    } else {
      clusters_to_split.insert(src_shared_id);
    }
  }

  double best_c_try = -1.0;
  std::set<ObjIDType> best_selected;
  SharedIDType best_id = -1;
  for (auto const& shared_id : clusters_to_split) {
    auto const& [src_cluster_bytes, src_cluster_load] = cur_clusters_[shared_id];

    std::set<ObjLoad> objs;
    for (auto const& [obj_id, shared_id_obj] : obj_shared_block_) {
      if (shared_id_obj == shared_id) {
        objs.emplace(obj_id, cur_objs_[obj_id]);
      }
    }

    std::set<ObjIDType> selected;
    LoadType load_sum = 0;
    for (auto const& [obj_id, load] : objs) {
      load_sum += load;
      selected.insert(obj_id);

      // We will not consider empty cluster "swaps" here.
      if (selected.size() != objs.size()) {
        auto src_cluster_bytes_add =
          try_clusters.find(shared_id) == try_clusters.end() ? src_cluster_bytes : 0;

        double c_try = criterion(
          std::make_tuple(shared_id, src_cluster_bytes_add, load_sum),
          std::make_tuple(try_rank, try_load, try_total_bytes)
        );

        vt_debug_print(
          terse, temperedlb,
          "testing a possible sub-cluster (rank {}): id={} load={} c_try={}, "
          "amount over average={}, amount under average={}\n",
          try_rank, shared_id, load_sum, c_try, amount_over_average,
          amount_under_average
        );

        if (c_try > 0.0) {
          best_c_try = c_try;
          best_selected = selected;
          best_id = shared_id;
        }
      }
    }
  }

  if (best_c_try > 0.0) {
    vt_debug_print(
      normal, temperedlb,
      "best_c_try={}, picked subcluster with id={} for rank ={}\n",
      best_c_try, best_id, try_rank
    );

    auto const& [
      give_objs,
      give_obj_shared_block,
      give_shared_blocks_size,
      give_obj_working_bytes
    ] = removeClusterToSend(best_id, best_selected);

    auto const this_node = theContext()->getNode();

    runInEpochRooted("giveSubCluster", [&]{
      proxy_[try_rank].template send<&TemperedLB::giveCluster>(
        this_node,
        give_shared_blocks_size,
        give_objs,
        give_obj_shared_block,
        give_obj_working_bytes,
        -1
      );
    });

    computeClusterSummary();

    vt_debug_print(
      normal, temperedlb,
      "best_c_try={}, sub-cluster sent to rank={}\n",
      best_c_try, try_rank
    );
  }

  proxy_[try_rank].template send<&TemperedLB::releaseLock>();

  is_swapping_ = false;

  if (pending_actions_.size() > 0) {
    auto action = pending_actions_.back();
    pending_actions_.pop_back();
    action();
  }
}

void TemperedLB::considerSwapsAfterLock(MsgSharedPtr<LockedInfoMsg> msg) {
  is_swapping_ = true;

  auto criterion = [&,this](auto src_cluster, auto try_cluster) -> double {
    auto const& [src_id, src_bytes, src_load] = src_cluster;
    auto const& [try_rank, try_total_load, try_total_bytes,
                 try_id, try_bytes, try_load] = try_cluster;

    auto const src_after_mem = current_memory_usage_ - src_bytes + try_bytes;
    auto const try_after_mem = try_total_bytes + src_bytes - try_bytes;

    // Check whether strict bounds on memory are satisfied
    if (src_after_mem > mem_thresh_ or try_after_mem > mem_thresh_) {
      return - std::numeric_limits<double>::infinity();
    }

    // Return load transfer criterion
    return loadTransferCriterion(this_new_load_, try_total_load, src_load, try_load);
  };

  auto const& try_clusters = msg->locked_clusters;
  auto const& try_rank = msg->locked_node;
  auto const& try_load = msg->locked_load;
  auto const& try_total_bytes = msg->locked_bytes;

  double best_c_try = -1.0;
  std::tuple<SharedIDType, SharedIDType> best_swap = {-1,-1};
  for (auto const& [src_shared_id, src_cluster] : cur_clusters_) {
    auto const& [src_cluster_bytes, src_cluster_load] = src_cluster;

    // try swapping with empty cluster first
    {
        double c_try = criterion(
          std::make_tuple(src_shared_id, src_cluster_bytes, src_cluster_load),
          std::make_tuple(
            try_rank,
            try_load,
            try_total_bytes,
            -1,
            0,
            0
          )
        );
        if (c_try > 0.0) {
          if (c_try > best_c_try) {
            best_c_try = c_try;
            best_swap = std::make_tuple(src_shared_id, -1);
          }
        }
    }

    for (auto const& [try_shared_id, try_cluster] : try_clusters) {
      auto const& [try_cluster_bytes, try_cluster_load] = try_cluster;
        double c_try = criterion(
          std::make_tuple(src_shared_id, src_cluster_bytes, src_cluster_load),
          std::make_tuple(
            try_rank,
            try_load,
            try_total_bytes,
            try_shared_id,
            try_cluster_bytes,
            try_cluster_load
          )
        );
        vt_debug_print(
          verbose, temperedlb,
          "testing a possible swap (rank {}): {} {} c_try={}\n",
          try_rank, src_shared_id, try_shared_id, c_try
        );
        if (c_try > 0.0) {
          if (c_try > best_c_try) {
            best_c_try = c_try;
            best_swap = std::make_tuple(src_shared_id, try_shared_id);
          }
        }
    }
  }

  if (best_c_try > 0) {
    auto const& [src_shared_id, try_shared_id] = best_swap;

    vt_debug_print(
      normal, temperedlb,
      "best_c_try={}, swapping {} for {} on rank ={}\n",
      best_c_try, src_shared_id, try_shared_id, try_rank
    );

    auto const& [
      give_objs,
      give_obj_shared_block,
      give_shared_blocks_size,
      give_obj_working_bytes
    ] = removeClusterToSend(src_shared_id);

    auto const this_node = theContext()->getNode();

    runInEpochRooted("giveCluster", [&]{
      proxy_[try_rank].template send<&TemperedLB::giveCluster>(
        this_node,
        give_shared_blocks_size,
        give_objs,
        give_obj_shared_block,
        give_obj_working_bytes,
        try_shared_id
      );
    });

    computeClusterSummary();

    vt_debug_print(
      normal, temperedlb,
      "best_c_try={}, swap completed with rank={}\n",
      best_c_try, try_rank
    );
  }

  proxy_[try_rank].template send<&TemperedLB::releaseLock>();

  is_swapping_ = false;

  if (pending_actions_.size() > 0) {
    auto action = pending_actions_.back();
    pending_actions_.pop_back();
    action();
  }
}

void TemperedLB::giveCluster(
  NodeType from_rank,
  std::unordered_map<SharedIDType, BytesType> const& give_shared_blocks_size,
  std::unordered_map<ObjIDType, LoadType> const& give_objs,
  std::unordered_map<ObjIDType, SharedIDType> const& give_obj_shared_block,
  std::unordered_map<ObjIDType, BytesType> const& give_obj_working_bytes,
  SharedIDType take_cluster
) {
  n_transfers_swap_++;

  vtAssert(give_shared_blocks_size.size() == 1, "Must be one block right now");

  for (auto const& [obj_id, obj_load] : give_objs) {
    this_new_load_ += obj_load;
    cur_objs_[obj_id] = obj_load;
  }
  for (auto const& [id, bytes] : give_shared_blocks_size) {
    shared_block_size_[id] = bytes;
  }
  for (auto const& [obj_id, id] : give_obj_shared_block) {
    obj_shared_block_[obj_id] = id;
  }
  for (auto const& elm : give_obj_working_bytes) {
    obj_working_bytes_.emplace(elm);
  }

  if (take_cluster != -1) {
    auto const this_node = theContext()->getNode();

    auto const& [
      take_objs,
      take_obj_shared_block,
      take_shared_blocks_size,
      take_obj_working_bytes
    ] = removeClusterToSend(take_cluster);

    proxy_[from_rank].template send<&TemperedLB::giveCluster>(
      this_node,
      take_shared_blocks_size,
      take_objs,
      take_obj_shared_block,
      take_obj_working_bytes,
      -1
    );
  }

  computeClusterSummary();

  vt_debug_print(
    normal, temperedlb,
    "giveCluster: total memory usage={}, shared blocks here={}, "
    "memory_threshold={}, give_cluster={}, take_cluster={}\n", computeMemoryUsage(),
    getSharedBlocksHere().size(), mem_thresh_,
    give_shared_blocks_size.begin()->first, take_cluster
  );
}

void TemperedLB::releaseLock() {
  vt_debug_print(
    normal, temperedlb,
    "releaseLock: pending size={}\n",
    pending_actions_.size()
  );

  is_locked_ = false;
  locking_rank_ = uninitialized_destination;

  if (pending_actions_.size() > 0) {
    auto action = pending_actions_.back();
    pending_actions_.pop_back();
    action();
  } else {
    // satisfy another lock
    satisfyLockRequest();
  }
}

void TemperedLB::lockObtained(LockedInfoMsg* in_msg) {
  auto msg = promoteMsg(in_msg);

  vt_debug_print(
    normal, temperedlb,
    "lockObtained: is_locked_={}, is_subclustering_={}\n",
    is_locked_, is_subclustering_
  );

  auto cur_epoch = theMsg()->getEpoch();
  theTerm()->produce(cur_epoch);

  auto action = [this, msg, cur_epoch]{
    theMsg()->pushEpoch(cur_epoch);
    if (is_subclustering_) {
      considerSubClustersAfterLock(msg);
    } else {
      considerSwapsAfterLock(msg);
    }
    theMsg()->popEpoch(cur_epoch);
    theTerm()->consume(cur_epoch);
  };

  if (is_locked_ && locking_rank_ <= msg->locked_node) {
    proxy_[msg->locked_node].template send<&TemperedLB::releaseLock>();
    theTerm()->consume(cur_epoch);
    try_locks_.emplace(msg->locked_node, msg->locked_c_try);
    //pending_actions_.push_back(action);
  } else if (is_locked_) {
    pending_actions_.push_back(action);
  } else if (is_swapping_) {
    pending_actions_.push_back(action);
  } else {
    action();
  }
}

void TemperedLB::satisfyLockRequest() {
  vtAssert(not is_locked_, "Must not already be locked to satisfy a request");
  if (try_locks_.size() > 0) {
    // find the best lock to give
    for (auto&& tl : try_locks_) {
      vt_debug_print(
        verbose, temperedlb,
        "satisfyLockRequest: node={}, c_try={}\n", tl.requesting_node, tl.c_try
      );
    }

    auto iter = try_locks_.begin();
    auto lock = *iter;
    try_locks_.erase(iter);

    auto const this_node = theContext()->getNode();

    vt_debug_print(
      normal, temperedlb,
      "satisfyLockRequest: locked obtained for node={}\n",
      lock.requesting_node
    );

    proxy_[lock.requesting_node].template send<&TemperedLB::lockObtained>(
      this_node, this_new_load_, cur_clusters_, current_memory_usage_,
      max_object_working_bytes_, lock.c_try
    );

    is_locked_ = true;
    locking_rank_ = lock.requesting_node;
  }
}

void TemperedLB::trySubClustering() {
  is_subclustering_ = true;
  n_transfers_swap_ = 0;

  auto lazy_epoch = theTerm()->makeEpochCollective("TemperedLB: subCluster");
  theTerm()->pushEpoch(lazy_epoch);

  auto const this_node = theContext()->getNode();

  vt_print(
    temperedlb,
    "SUBcluster: load={} max_load={}\n",
    this_new_load_, max_load_over_iters_.back()
  );

  // Only ranks that are close to max should do this...otherwise its a waste
  // Very aggressive to start.
  if (
    auto n_iters = max_load_over_iters_.size();
    this_new_load_ / max_load_over_iters_[n_iters - 1] > 0.80
  ) {
    BytesType avg_cluster_bytes = 0;
    for (auto const& [src_shared_id, src_cluster] : cur_clusters_) {
      auto const& [src_cluster_bytes, src_cluster_load] = src_cluster;
      avg_cluster_bytes += src_cluster_bytes;
    }
    avg_cluster_bytes /= cur_clusters_.size();

    for (auto const& [try_rank, try_clusters] : other_rank_clusters_) {

      BytesType total_clusters_bytes = 0;
      for (auto const& [try_shared_id, try_cluster] : try_clusters) {
        auto const& [try_cluster_bytes, try_cluster_load] = try_cluster;
        total_clusters_bytes += try_cluster_bytes;
      }

      vt_print(
        temperedlb,
        "SUBcluster: load={} max_load={}, try_rank={}\n",
        this_new_load_, max_load_over_iters_.back(), try_rank
      );


      // Only target ranks where the target rank has room for the average
      // cluster size that this rank has
      if (total_clusters_bytes + avg_cluster_bytes < mem_thresh_) {
        if (
          auto target_rank_load = load_info_.find(try_rank)->second;
          target_rank_load < target_max_load_
        ) {

          vt_print(
            temperedlb,
            "SUBcluster: load={} max_load={}, try_rank={} sending lock\n",
            this_new_load_, max_load_over_iters_.back(), try_rank
          );

          // c-value is now the ratio of load compared to this rank. prefer
          // ranks that have less load and have fewer clusters.
          proxy_[try_rank].template send<&TemperedLB::tryLock>(
            this_node, this_new_load_ / target_rank_load
          );
        }
      }

    }

  } else {
    // do nothing--not loaded enough, may be a target to put load
  }

  // We have to be very careful here since we will allow some reentrancy here.
  constexpr int turn_scheduler_times = 10;
  for (int i = 0; i < turn_scheduler_times; i++) {
    theSched()->runSchedulerOnceImpl();
  }

  while (not theSched()->workQueueEmpty()) {
    theSched()->runSchedulerOnceImpl();
  }

  ready_to_satisfy_locks_ = true;
  satisfyLockRequest();

  // Finalize epoch, we have sent our initial round of messages
  // from here everything is message driven
  theTerm()->finishedEpoch(lazy_epoch);
  theTerm()->popEpoch(lazy_epoch);
  vt::runSchedulerThrough(lazy_epoch);

  vt_debug_print(
    normal, temperedlb,
    "After subclustering iteration: total memory usage={}, shared blocks here={}, "
    "memory_threshold={}, load={}\n", computeMemoryUsage(),
    getSharedBlocksHere().size(), mem_thresh_, this_new_load_
  );

  int n_rejected = 0;

  // Report on rejection rate in debug mode
  if (theConfig()->vt_debug_temperedlb) {
    runInEpochCollective("TemperedLB::swapClusters -> compute rejection", [=] {
      proxy_.allreduce<&TemperedLB::rejectionStatsHandler, collective::PlusOp>(
        n_rejected, n_transfers_swap_
      );
    });
  }
}

void TemperedLB::swapClusters() {
#if 0
  // Do the test to see if we should start sub-clustering. This is probably far
  // too aggressive. We could check as an conservative check that requires more
  // computation to see if a cluster is blocking progress.
  if (auto const len = max_load_over_iters_.size(); len > 2) {
    double const i1 = max_load_over_iters_[len-1];
    double const i2 = max_load_over_iters_[len-2];

    vt_debug_print(
      terse, temperedlb,
      "swapClusters: check for subclustering: i1={}, i2={},"
      " criteria=abs={} tol={}\n",
      i1, i2, std::abs(i1 - i2), 0.01*i1
    );

    // the max is mostly stable
    if (std::abs(i1 - i2) < 0.01*i1) {
      trySubClustering();
      return;
    }
  }
#endif

  n_transfers_swap_ = 0;

  auto lazy_epoch = theTerm()->makeEpochCollective("TemperedLB: swapClusters");
  theTerm()->pushEpoch(lazy_epoch);

  auto criterion = [this](auto src_cluster, auto try_cluster) -> double {
    // FIXME: this does not swaps with an empty cluster
    auto const& [src_id, src_bytes, src_load] = src_cluster;
    auto const& [try_rank, try_id, try_bytes, try_load, try_mem] = try_cluster;

    // Necessary but not sufficient check regarding memory bounds
    if (try_mem - try_bytes + src_bytes > mem_thresh_) {
      return - std::numeric_limits<double>::infinity();
    }

    // Return load transfer criterion
    return loadTransferCriterion(this_new_load_, load_info_.find(try_rank)->second, src_load, try_load);
  };

  auto const this_node = theContext()->getNode();

  // Identify and message beneficial cluster swaps
  for (auto const& [try_rank, try_clusters] : other_rank_clusters_) {
    bool found_potential_good_swap = false;

    // Approximate roughly the memory usage on the target
    BytesType try_approx_mem_usage = rank_bytes_;
    for (auto const& [try_shared_id, try_cluster] : try_clusters) {
      auto const& [try_cluster_bytes, _] = try_cluster;
      try_approx_mem_usage += try_cluster_bytes;
    }

    // Iterate over source clusters
    for (auto const& [src_shared_id, src_cluster] : cur_clusters_) {
      auto const& [src_cluster_bytes, src_cluster_load] = src_cluster;

      // Compute approximation swap criterion for empty cluster "swap" case
      {
        double c_try = criterion(
          std::make_tuple(src_shared_id, src_cluster_bytes, src_cluster_load),
          std::make_tuple(try_rank, 0, 0, 0, try_approx_mem_usage)
        );
        if (c_try > 0.0) {
	  // Try to obtain lock for feasible swap
          found_potential_good_swap = true;
          proxy_[try_rank].template send<&TemperedLB::tryLock>(this_node, c_try);
          break;
        }
      }

      // Iterate over target clusters
      for (auto const& [try_shared_id, try_cluster] : try_clusters) {
        auto const& [try_cluster_bytes, try_cluster_load] = try_cluster;
	// Decide whether swap is beneficial
        double c_try = criterion(
          std::make_tuple(src_shared_id, src_cluster_bytes, src_cluster_load),
          std::make_tuple(
            try_rank, try_shared_id, try_cluster_bytes, try_cluster_load,
            try_approx_mem_usage
          )
        );
        if (c_try > 0.0) {
	  // Try to obtain lock for feasible swap
          found_potential_good_swap = true;
          proxy_[try_rank].template send<&TemperedLB::tryLock>(this_node, c_try);
          break;
        }
      } // try_clusters
      if (found_potential_good_swap) {
        break;
      }
    } // cur_clusters_
  } // other_rank_clusters

  // We have to be very careful here since we will allow some reentrancy here.
  constexpr int turn_scheduler_times = 10;
  for (int i = 0; i < turn_scheduler_times; i++) {
    theSched()->runSchedulerOnceImpl();
  }

  while (not theSched()->workQueueEmpty()) {
    theSched()->runSchedulerOnceImpl();
  }

  ready_to_satisfy_locks_ = true;
  satisfyLockRequest();

  // Finalize epoch, we have sent our initial round of messages
  // from here everything is message driven
  theTerm()->finishedEpoch(lazy_epoch);
  theTerm()->popEpoch(lazy_epoch);
  vt::runSchedulerThrough(lazy_epoch);

  vt_debug_print(
    normal, temperedlb,
    "After iteration: total memory usage={}, shared blocks here={}, "
    "memory_threshold={}, load={}\n", computeMemoryUsage(),
    getSharedBlocksHere().size(), mem_thresh_, this_new_load_
  );


  // Report on rejection rate in debug mode
  int n_rejected = 0;
  if (theConfig()->vt_debug_temperedlb) {
    runInEpochCollective("TemperedLB::swapClusters -> compute rejection", [=] {
      proxy_.allreduce<&TemperedLB::rejectionStatsHandler, collective::PlusOp>(
        n_rejected, n_transfers_swap_
      );
    });
  }
} // void TemperedLB::originalTransfer()

void TemperedLB::thunkMigrations() {
  vt_debug_print(
    normal, temperedlb,
    "thunkMigrations, total num_objs={}\n",
    cur_objs_.size()
  );

  auto this_node = theContext()->getNode();
  for (auto elm : cur_objs_) {
    auto obj = elm.first;
    migrateObjectTo(obj, this_node);
  }
}

void TemperedLB::inLazyMigrations(balance::LazyMigrationMsg* msg) {
  auto const& incoming_objs = msg->getObjSet();
  for (auto& obj : incoming_objs) {
    auto iter = cur_objs_.find(obj.first);
    vtAssert(iter == cur_objs_.end(), "Incoming object should not exist");
    cur_objs_.insert(obj);
    // need to convert to milliseconds because we received seconds
    this_new_load_ += obj.second;
  }
}

void TemperedLB::lazyMigrateObjsTo(
  EpochType epoch, NodeType node, ObjsType const& objs
) {
  using LazyMsg = balance::LazyMigrationMsg;
  auto msg = makeMessage<LazyMsg>(node, objs);
  envelopeSetEpoch(msg->env, epoch);
  proxy_[node].sendMsg<LazyMsg, &TemperedLB::inLazyMigrations>(msg);
}

void TemperedLB::migrate() {
  vtAssertExpr(false);
}

LoadType TemperedLB::getModeledValue(const elm::ElementIDStruct& obj) {
  return load_model_->getModeledLoad(
    obj, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}
  );
}

}}}} /* end namespace vt::vrt::collection::lb */
