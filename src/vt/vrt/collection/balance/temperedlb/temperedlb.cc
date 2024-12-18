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
      footprint, in contrast with whole cluster swaps.
)"
    },
    {
      "ordering",
      R"(
Values: {Arbitrary, ElmID, FewestMigrations, SmallObjects, LargestObjects}
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
Values: {Original, NormByMax, NormByMaxExcludeIneligible}
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
  If transfer_strategy is SwapClusters, rollback is automatically set to false.
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
    },
    {
      "alpha",
      R"(
Values: <double>
Defaut: 1.0
Description: α in the work model (load in work model)
)"
    },
    {
      "beta",
      R"(
Values: <double>
Defaut: 0.0
Description: β in the work model (inter-node communication in work model)
)"
    },
    {
    "gamma",
      R"(
Values: <double>
Defaut: 0.0
Description: γ in the work model (intra-node communication in work model)
)"
    },
    {
    "delta",
      R"(
Values: <double>
Defaut: 0.0
Description: δ in the work model (shared-memory-edges in work model)
)"
    },
    {
    "epsilon",
      R"(
Values: <double>
Defaut: infinity
Description: ε in the work model (memory term in work model)
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

  alpha = config->getOrDefault<double>("alpha", alpha);
  beta = config->getOrDefault<double>("beta", beta);
  gamma = config->getOrDefault<double>("gamma", gamma);
  delta = config->getOrDefault<double>("delta", delta);
  epsilon = config->getOrDefault<double>("epsilon", epsilon);

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

  if (transfer_type_ == TransferTypeEnum::SwapClusters) {
    rollback_ = false;
  }

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
      "transfer={}, ordering={}, cmf={}, rollback={}, targetpole={}\n",
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

  if (maxLoadExceedsLBCost()) {
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
#if vt_check_enabled(trace_enabled)
    theTrace()->disableTracing();
#endif

    runInEpochCollective("doLBStages", [&,this]{
      auto this_node = theContext()->getNode();
      proxy_[this_node].template send<&TemperedLB::doLBStages>(imb);
    });

#if vt_check_enabled(trace_enabled)
    theTrace()->enableTracing();
#endif
  }
}

void TemperedLB::readClustersMemoryData() {
  if (load_model_->hasUserData()) {
    for (auto obj : *load_model_) {
      if (obj.isMigratable()) {
        auto data_map = load_model_->getUserData(
          obj, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}
        );

        SharedIDType shared_id = vt::no_shared_id;
        vt::NodeType home_rank = vt::uninitialized_destination;
        BytesType shared_bytes = 0;
        BytesType working_bytes = 0;
        BytesType footprint_bytes = 0;
        BytesType serialized_bytes = 0;
        for (auto const& [key, variant] : data_map) {
          auto val = std::get_if<double>(&variant);
          vtAbortIf(!val, '"' + key + "\" in variant does not match double");

          if (key == "shared_id") {
            // Because of how JSON is stored this is always a double, even
            // though it should be an integer
            shared_id = static_cast<int>(*val);
          }
          if (key == "home_rank") {
            // Because of how JSON is stored this is always a double, even
            // though it should be an integer
            home_rank = static_cast<int>(*val);
          }
          if (key == "shared_bytes") {
            shared_bytes = *val;
          }
          if (key == "task_working_bytes") {
            working_bytes = *val;
          }
          if (key == "task_footprint_bytes") {
            footprint_bytes = *val;
          }
          if (key == "task_serialized_bytes") {
            serialized_bytes = *val;
          }
          if (key == "rank_working_bytes") {
            rank_bytes_ = *val;
          }
        }

        vt_debug_print(
          verbose, temperedlb,
          "obj={} sid={} bytes={} footprint={} serialized={}, working={}\n",
          obj, shared_id, shared_bytes, footprint_bytes, serialized_bytes,
          working_bytes
        );

        obj_shared_block_[obj] = shared_id;
        obj_working_bytes_[obj] = working_bytes;
        obj_footprint_bytes_[obj] = footprint_bytes;
        obj_serialized_bytes_[obj] = serialized_bytes;
        shared_block_size_[shared_id] = shared_bytes;
        shared_block_edge_[shared_id] = std::make_tuple(home_rank, shared_bytes);
      }
    }
  }
}

void TemperedLB::computeClusterSummary() {
  cur_clusters_.clear();

  auto const this_node = theContext()->getNode();

  for (auto const& [shared_id, shared_bytes] : shared_block_size_) {
    auto const& [home_node, shared_volume] = shared_block_edge_[shared_id];

    ClusterInfo info;
    info.bytes = shared_bytes;
    info.home_node = home_node;
    info.edge_weight = shared_volume;

    std::set<ObjIDType> cluster_objs;
    BytesType max_object_working_bytes = 0;
    BytesType max_object_working_bytes_outside = 0;
    BytesType max_object_serialized_bytes = 0;
    BytesType max_object_serialized_bytes_outside = 0;
    BytesType cluster_footprint = 0;

    for (auto const& [obj_id, obj_load] : cur_objs_) {
      if (auto iter = obj_shared_block_.find(obj_id); iter != obj_shared_block_.end()) {
        if (iter->second == shared_id) {
          cluster_objs.insert(obj_id);
          info.load += obj_load;
          if (
            auto it = obj_working_bytes_.find(obj_id);
            it != obj_working_bytes_.end()
          ) {
            max_object_working_bytes = std::max(
              max_object_working_bytes, it->second
            );
          }
          if (
            auto it = obj_serialized_bytes_.find(obj_id);
            it != obj_serialized_bytes_.end()
          ) {
            max_object_serialized_bytes = std::max(
              max_object_serialized_bytes, it->second
            );
          }
          if (
            auto it = obj_footprint_bytes_.find(obj_id);
            it != obj_footprint_bytes_.end()
          ) {
            cluster_footprint += it->second;
          }
        } else {
          if (
            auto it = obj_working_bytes_.find(obj_id);
            it != obj_working_bytes_.end()
          ) {
            max_object_working_bytes_outside = std::max(
              max_object_working_bytes_outside, it->second
            );
          }
          if (
            auto it = obj_serialized_bytes_.find(obj_id);
            it != obj_serialized_bytes_.end()
          ) {
            max_object_serialized_bytes_outside = std::max(
              max_object_serialized_bytes_outside, it->second
            );
          }
        }
      }
    }

    info.cluster_footprint = cluster_footprint;
    info.max_object_working_bytes = max_object_working_bytes;
    info.max_object_working_bytes_outside = max_object_working_bytes_outside;
    info.max_object_serialized_bytes = max_object_serialized_bytes;
    info.max_object_serialized_bytes_outside = max_object_serialized_bytes_outside;

    if (info.load != 0) {
      for (auto&& obj : cluster_objs) {
        if (auto it = send_edges_.find(obj); it != send_edges_.end()) {
          for (auto const& [target, volume] : it->second) {
            vt_debug_print(
              verbose, temperedlb,
              "computeClusterSummary: send obj={}, target={}\n",
              obj, target
            );

            if (cluster_objs.find(target) != cluster_objs.end()) {
              // intra-cluster edge
              info.intra_send_vol += volume;
            } else if (
              cur_objs_.find(target) != cur_objs_.end() or
              target.isLocatedOnThisNode()
            ) {
              // intra-rank edge
              info.inter_send_vol[this_node] += volume;
            } else {
              // inter-rank edge
              info.inter_send_vol[target.getCurrNode()] += volume;
            }
          }
        }
        if (auto it = recv_edges_.find(obj); it != recv_edges_.end()) {
          for (auto const& [target, volume] : it->second) {
            vt_debug_print(
              verbose, temperedlb,
              "computeClusterSummary: recv obj={}, target={}\n",
              obj, target
            );
            if (cluster_objs.find(target) != cluster_objs.end()) {
              // intra-cluster edge
              info.intra_recv_vol += volume;
            } else if (
              cur_objs_.find(target) != cur_objs_.end() or
              target.isLocatedOnThisNode()
            ) {
              // intra-rank edge
              info.inter_recv_vol[this_node] += volume;
            } else {
              // inter-rank edge
              info.inter_recv_vol[target.getCurrNode()] += volume;
            }
          }
        }
      }

      cur_clusters_.emplace(shared_id, std::move(info));
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

  // Compute max object working and serialized bytes
  for (auto const& [obj_id, _] : cur_objs_)  {
    if (
      auto it = obj_serialized_bytes_.find(obj_id);
      it != obj_serialized_bytes_.end()
    ) {
      max_object_serialized_bytes_ =
        std::max(max_object_serialized_bytes_, it->second);
    }
    if (
      auto it = obj_working_bytes_.find(obj_id);
      it != obj_working_bytes_.end()
    ) {
      max_object_working_bytes_ =
        std::max(max_object_working_bytes_, it->second);
    } else {
      vt_debug_print(
        verbose, temperedlb,
        "Warning: working bytes not found for object: {}\n", obj_id
      );
    }
  }

  // Sum up all footprint bytes
  double object_footprint_bytes = 0;
  for (auto const& [obj_id, _] : cur_objs_)  {
    if (
      auto it = obj_footprint_bytes_.find(obj_id);
      it != obj_footprint_bytes_.end()
    ) {
      object_footprint_bytes += it->second;
    }
  }

  return current_memory_usage_ =
    rank_bytes_ +
    total_shared_bytes +
    max_object_working_bytes_ +
    object_footprint_bytes +
    max_object_serialized_bytes_;
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

int TemperedLB::getRemoteBlockCountHere() const {
  auto this_node = theContext()->getNode();
  auto const& shared_blocks_here = getSharedBlocksHere();
  int remote_block_count = 0;
  for (auto const& sid : shared_blocks_here) {
    if (auto it = shared_block_edge_.find(sid); it != shared_block_edge_.end()) {
      auto const& [home_node, volume] = it->second;
      if (home_node != this_node) {
        remote_block_count++;
      }
    } else {
      vtAbort("Could not find shared edge volume!");
    }
  }
  return remote_block_count;
}

void TemperedLB::workStatsHandler(std::vector<balance::LoadData> const& vec) {
  auto const& work = vec[1];
  work_mean_ = work.avg();
  work_max_ = work.max();
  new_work_imbalance_ = work.I();
}

double TemperedLB::computeWork(
  double load, double inter_comm_bytes, double intra_comm_bytes,
  double shared_comm_bytes
) const {
  // The work model based on input parameters (excluding epsilon)
  return
    alpha * load +
    beta * inter_comm_bytes +
    gamma * intra_comm_bytes +
    delta * shared_comm_bytes;
}

WorkBreakdown TemperedLB::computeWorkBreakdown(
  NodeType node,
  std::unordered_map<ObjIDType, LoadType> const& objs,
  std::set<ObjIDType> const& exclude,
  std::unordered_map<ObjIDType, LoadType> const& include
) {
  double load = 0;

  // Communication bytes sent/recv'ed within the rank
  double intra_rank_bytes_sent = 0, intra_rank_bytes_recv = 0;
  // Communication bytes sent/recv'ed off rank
  double inter_rank_bytes_sent = 0, inter_rank_bytes_recv = 0;

  auto computeEdgeVolumesAndLoad = [&](ObjIDType obj, LoadType obj_load) {
    if (exclude.find(obj) == exclude.end()) {
      if (auto it = send_edges_.find(obj); it != send_edges_.end()) {
        for (auto const& [target, volume] : it->second) {
          vt_debug_print(
            verbose, temperedlb,
            "computeWorkBreakdown: send obj={}, target={}\n",
            obj, target
          );
          if (
            cur_objs_.find(target) != cur_objs_.end() or
            target.isLocatedOnThisNode()
          ) {
            intra_rank_bytes_sent += volume;
          } else {
            inter_rank_bytes_sent += volume;
          }
        }
      }
      if (auto it = recv_edges_.find(obj); it != recv_edges_.end()) {
        for (auto const& [target, volume] : it->second) {
          vt_debug_print(
            verbose, temperedlb,
            "computeWorkBreakdown: recv obj={}, target={}\n",
            obj, target
          );
          if (
            cur_objs_.find(target) != cur_objs_.end() or
            target.isLocatedOnThisNode()
          ) {
            intra_rank_bytes_recv += volume;
          } else {
            inter_rank_bytes_recv += volume;
          }
        }
      }
    }

    load += obj_load;
  };

  for (auto const& [obj, obj_load] : objs) {
    computeEdgeVolumesAndLoad(obj, obj_load);
  }

  for (auto const& [obj, obj_load] : include) {
    computeEdgeVolumesAndLoad(obj, obj_load);
  }

  double shared_volume = 0;
  auto const& shared_blocks_here = getSharedBlocksHere();

  for (auto const& sid : shared_blocks_here) {
    if (auto it = shared_block_edge_.find(sid); it != shared_block_edge_.end()) {
      auto const& [home_node, volume] = it->second;
      if (home_node != node) {
        shared_volume += volume;
      }
    } else {
      vtAbort("Could not find shared edge volume!");
    }
  }

  auto const inter_vol = std::max(inter_rank_bytes_sent, inter_rank_bytes_recv);
  auto const intra_vol = std::max(intra_rank_bytes_sent, intra_rank_bytes_recv);

  WorkBreakdown w;
  w.work = computeWork(load, inter_vol, intra_vol, shared_volume);
  w.intra_send_vol = intra_rank_bytes_sent;
  w.intra_recv_vol = intra_rank_bytes_recv;
  w.inter_send_vol = inter_rank_bytes_sent;
  w.inter_recv_vol = inter_rank_bytes_recv;
  w.shared_vol = shared_volume;

  vt_debug_print(
    normal, temperedlb,
    "computeWorkBreakdown: load={}, intra sent={}, recv={},"
    " inter sent={}, recv={}, shared_vol={}, work={}\n",
    load,
    intra_rank_bytes_sent, intra_rank_bytes_recv,
    inter_rank_bytes_sent, inter_rank_bytes_recv,
    shared_volume, w.work
  );

  return w;
}

double TemperedLB::computeWorkAfterClusterSwap(
  NodeType node, NodeInfo const& info, ClusterInfo const& to_remove,
  ClusterInfo const& to_add
) {
  // Start with the existing work for the node and work backwards to compute the
  // new work with the cluster removed
  double node_work = info.work;

  // Remove/add clusters' load factor from work model
  node_work -= alpha * to_remove.load;
  node_work += alpha * to_add.load;

  // Remove/add clusters' intra-comm
  double const node_intra_send = info.intra_send_vol;
  double const node_intra_recv = info.intra_recv_vol;
  node_work -= gamma * std::max(node_intra_send, node_intra_recv);
  node_work += gamma * std::max(
    node_intra_send - to_remove.intra_send_vol + to_add.intra_send_vol,
    node_intra_recv - to_remove.intra_recv_vol + to_add.intra_recv_vol
  );

  // Uninitialized destination means that the cluster is empty
  // If to_remove was remote, remove that component from the work
  if (
    to_remove.home_node != node and
    to_remove.home_node != uninitialized_destination
  ) {
    node_work -= delta * to_remove.edge_weight;
  }

  // If to_add is now remote, add that component to the work
  if (
    to_add.home_node != node and
    to_add.home_node != uninitialized_destination
  ) {
    node_work += delta * to_add.edge_weight;
  }

  // Update formulae for inter-node communication
  double node_inter_send = info.inter_send_vol;
  double node_inter_recv = info.inter_recv_vol;
  node_work -= beta * std::max(node_inter_send, node_inter_recv);

  // All edges outside the to_remove cluster that are also off the node need to
  // be removed from the inter-node volumes
  for (auto const& [target, volume] : to_remove.inter_send_vol) {
    if (target != node) {
      node_inter_send -= volume;
    }
  }
  for (auto const& [target, volume] : to_remove.inter_recv_vol) {
    if (target != node) {
      node_inter_recv -= volume;
    }
  }

  // All edges outside the to_add cluster that are now off the node need to
  // be added from the inter-node volumes
  for (auto const& [target, volume] : to_add.inter_send_vol) {
    if (target != node) {
      node_inter_send += volume;
    }
  }
  for (auto const& [target, volume] : to_add.inter_recv_vol) {
    if (target != node) {
      node_inter_recv += volume;
    }
  }

  node_work += beta * std::max(node_inter_send, node_inter_recv);

  return node_work;
}

void TemperedLB::doLBStages(LoadType start_imb) {
  decltype(this->cur_objs_) best_objs;
  LoadType best_load = 0;
  LoadType best_imb = start_imb + 10;
  uint16_t best_trial = 0;

  auto this_node = theContext()->getNode();

  // Read in memory information if it's available before we do any trials
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
    ready_to_satisfy_locks_ = false;

    LoadType best_imb_this_trial = start_imb + 10;

    for (iter_ = 0; iter_ < num_iters_; iter_++) {
      bool first_iter = iter_ == 0;

      if (first_iter) {
        // Copy this node's object assignments to a local, mutable copy
        cur_objs_.clear();
        int total_num_objs = 0;
        int num_migratable_objs = 0;
        for (auto obj : *load_model_) {
          total_num_objs++;
          if (obj.isMigratable()) {
            num_migratable_objs++;
            cur_objs_[obj] = getModeledValue(obj);
          }
        }

        vt_debug_print(
          normal, temperedlb,
          "TemperedLB::doLBStages: Found {} migratable objects out of {}.\n",
          num_migratable_objs, total_num_objs
        );

        send_edges_.clear();
        recv_edges_.clear();
        bool has_comm = false;
        auto const& comm = load_model_->getComm(
          {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}
        );
        // vt_print(temperedlb, "comm size={} {}\n", comm.size(), typeid(load_model_).name());

        for (auto const& [key, volume] : comm) {
          // vt_print(temperedlb, "Found comm: volume={}\n", volume.bytes);
          // Skip self edges
          if (key.selfEdge()) {
            continue;
          }

          if (key.commCategory() == elm::CommCategory::SendRecv) {
            auto const from_obj = key.fromObj();
            auto const to_obj = key.toObj();
            auto const bytes = volume.bytes;

            send_edges_[from_obj].emplace_back(to_obj, bytes);
            recv_edges_[to_obj].emplace_back(from_obj, bytes);
            has_comm = true;
          } else if (key.commCategory() == elm::CommCategory::WriteShared) {
            auto const to_node = key.toNode();
            auto const shared_id = key.sharedID();
            auto const bytes = volume.bytes;
            shared_block_edge_[shared_id] = std::make_tuple(to_node, bytes);
            has_comm = true;
          } else if (key.commCategory() == elm::CommCategory::ReadOnlyShared) {
            auto const to_node = key.toNode();
            auto const shared_id = key.sharedID();
            auto const bytes = volume.bytes;
            shared_block_edge_[shared_id] = std::make_tuple(to_node, bytes);
            has_comm = true;
          }
        }

        runInEpochCollective("checkIfEdgesExist", [&]{
          proxy_.allreduce<&TemperedLB::hasCommAny, collective::OrOp>(has_comm);
        });

        if (has_comm_any_) {
          runInEpochCollective("symmEdges", [&]{
            std::unordered_map<NodeType, EdgeMapType> edges;

            for (auto const& [from_obj, to_edges] : send_edges_) {
              for (auto const& [to_obj, volume] : to_edges) {
                vt_debug_print(
                  verbose, temperedlb,
                  "SymmEdges: from={}, to={}, volume={}\n",
                  from_obj, to_obj, volume
                );
                auto curr_from_node = from_obj.getCurrNode();
                if (curr_from_node != this_node) {
                  edges[curr_from_node][from_obj].emplace_back(to_obj, volume);
                }
                auto curr_to_node = to_obj.getCurrNode();
                if (curr_to_node != this_node) {
                  edges[curr_to_node][from_obj].emplace_back(to_obj, volume);
                }
              }
            }

            for (auto const& [dest_node, edge_map] : edges) {
              proxy_[dest_node].template send<&TemperedLB::giveEdges>(edge_map);
            }
          });
        }

        this_new_load_ = this_load;
        this_new_breakdown_ = computeWorkBreakdown(this_node, cur_objs_);
        this_work = this_new_work_ = this_new_breakdown_.work;

        runInEpochCollective("TemperedLB::doLBStages -> Rank_load_modeled", [=] {
          // Perform the reduction for Rank_load_modeled -> processor load only
          proxy_.allreduce<&TemperedLB::workStatsHandler, collective::PlusOp>(
            std::vector<balance::LoadData>{
              {balance::LoadData{Statistic::Rank_load_modeled, this_new_load_}},
              {balance::LoadData{Statistic::Rank_strategy_specific_load_modeled, this_new_work_}}
            }
          );
        });

      } else {
        // Clear out data structures from previous iteration
        selected_.clear();
        underloaded_.clear();
        load_info_.clear();
        is_overloaded_ = is_underloaded_ = false;
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
        double const memory_usage = computeMemoryUsage();

        vt_debug_print(
          normal, temperedlb,
          "Current memory info: total memory usage={}, shared blocks here={}, "
          "memory_threshold={}\n", memory_usage,
          getSharedBlocksHere().size(), mem_thresh_
        );

        if (memory_usage > mem_thresh_) {
          vtAbort("This should never be possible to go over the threshold\n");
        }

        computeClusterSummary();

        // Verbose printing about local clusters
        for (auto const& [shared_id, cluster_info] : cur_clusters_) {
          vt_debug_print(
            verbose, temperedlb,
            "Local cluster: id={}: {}\n",
            shared_id, cluster_info
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
        for (auto const& [shared_id, cluster_info] : clusters) {
          vt_debug_print(
            verbose, temperedlb,
            "Remote cluster: node={}, id={}, {}\n",
            node, shared_id, cluster_info
          );
        }
      }

      // Move remote cluster information to shared_block_size_ so we have all
      // the sizes in the same place
      for (auto const& [node, clusters] : other_rank_clusters_) {
        for (auto const& [shared_id, cluster_info] : clusters) {
          shared_block_size_[shared_id] = cluster_info.bytes;
          shared_block_edge_[shared_id] =
            std::make_tuple(cluster_info.home_node, cluster_info.edge_weight);
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
        this_new_breakdown_ = computeWorkBreakdown(this_node, cur_objs_);
        this_new_work_ = this_new_breakdown_.work;
        runInEpochCollective("TemperedLB::doLBStages -> Rank_load_modeled", [=] {
          // Perform the reduction for Rank_load_modeled -> processor load only
          proxy_.allreduce<&TemperedLB::loadStatsHandler, collective::PlusOp>(
            std::vector<balance::LoadData>{
              {balance::LoadData{Statistic::Rank_load_modeled, this_new_load_}},
              {balance::LoadData{Statistic::Rank_strategy_specific_load_modeled, this_new_work_}}
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
    send_edges_.clear();
    recv_edges_.clear();
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

    // Skip this block when not using SwapClusters
    if (transfer_type_ == TransferTypeEnum::SwapClusters) {
      auto remote_block_count = getRemoteBlockCountHere();
      runInEpochCollective("TemperedLB::doLBStages -> compute unhomed", [=] {
        proxy_.allreduce<&TemperedLB::remoteBlockCountHandler,
                         collective::PlusOp>(remote_block_count);
      });
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

void TemperedLB::giveEdges(EdgeMapType const& edge_map) {
  for (auto const& [from_obj, to_edges] : edge_map) {
    for (auto const& [to_obj, volume] : to_edges) {
      send_edges_[from_obj].emplace_back(to_obj, volume);
      recv_edges_[to_obj].emplace_back(from_obj, volume);
    }
  }
}

void TemperedLB::hasCommAny(bool has_comm_any) {
  has_comm_any_ = has_comm_any;
}

void TemperedLB::loadStatsHandler(std::vector<balance::LoadData> const& vec) {
  auto const& in = vec[0];
  auto const& work = vec[1];
  new_imbalance_ = in.I();

  work_mean_ = work.avg();
  work_max_ = work.max();
  new_work_imbalance_ = work.I();

  max_load_over_iters_.push_back(in.max());

  auto this_node = theContext()->getNode();
  if (this_node == 0) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::loadStatsHandler: trial={} iter={}"
      " Load[max={:0.2f} min={:0.2f} avg={:0.2f} pole={:0.2f} imb={:0.4f}] "
      " Work[max={:0.2f} min={:0.2f} avg={:0.2f} imb={:0.4f}]\n",
      trial_, iter_,
      LoadType(in.max()),
      LoadType(in.min()), LoadType(in.avg()),
      LoadType(stats.at(
        lb::Statistic::Object_load_modeled
      ).at(lb::StatisticQuantity::max)),
      in.I(),
      LoadType(work.max()),
      LoadType(work.min()), LoadType(work.avg()),
      work.I()
    );
  }
}

void TemperedLB::rejectionStatsHandler(
  int n_rejected, int n_transfers, int n_unhomed_blocks
) {
  double rej = static_cast<double>(n_rejected) /
    static_cast<double>(n_rejected + n_transfers) * 100.0;

  auto this_node = theContext()->getNode();
  if (this_node == 0) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::rejectionStatsHandler: n_transfers={} n_unhomed_blocks={}"
      " n_rejected={} "
      "rejection_rate={:0.1f}%\n",
      n_transfers, n_unhomed_blocks, n_rejected, rej
    );
  }
}

void TemperedLB::remoteBlockCountHandler(int n_unhomed_blocks) {
  auto this_node = theContext()->getNode();
  if (this_node == 0) {
    vt_print(
      temperedlb,
      "After load balancing, {} blocks will be off their home ranks\n",
      n_unhomed_blocks
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
      normal, temperedlb,
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
      NodeInfo info{
        this_new_load_, this_new_work_,
        this_new_breakdown_.inter_send_vol, this_new_breakdown_.inter_recv_vol,
        this_new_breakdown_.intra_send_vol, this_new_breakdown_.intra_recv_vol,
        this_new_breakdown_.shared_vol
      };
      msg->addNodeInfo(this_node, info);
      if (has_memory_data_) {
        msg->addNodeClusters(this_node, rank_bytes_, cur_clusters_);
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
      NodeInfo info{
        this_new_load_, this_new_work_,
        this_new_breakdown_.inter_send_vol, this_new_breakdown_.inter_recv_vol,
        this_new_breakdown_.intra_send_vol, this_new_breakdown_.intra_recv_vol,
        this_new_breakdown_.shared_vol
      };
      msg->addNodeInfo(this_node, info);
      if (has_memory_data_) {
        msg->addNodeClusters(this_node, rank_bytes_, cur_clusters_);
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
    trial_, iter_, k_max_, k_cur_async, from_node, msg->getNodeInfo().size()
  );

  auto const this_node = theContext()->getNode();
  for (auto const& [node, rank_summary] : msg->getNodeClusterSummary()) {
    if (
      node != this_node and
      other_rank_clusters_.find(node) == other_rank_clusters_.end()
    ) {
      auto const& [rank_working_bytes, clusters] = rank_summary;
      other_rank_clusters_[node] = clusters;
      other_rank_working_bytes_[node] = rank_working_bytes;
    }
  }

  for (auto&& elm : msg->getNodeInfo()) {
    if (load_info_.find(elm.first) == load_info_.end()) {
      load_info_[elm.first] = elm.second;

      if (isUnderloaded(elm.second.load)) {
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
    trial_, iter_, k_max_, k_cur_, from_node, msg->getNodeInfo().size()
  );

  auto const this_node = theContext()->getNode();
  for (auto const& [node, rank_summary] : msg->getNodeClusterSummary()) {
    if (
      node != this_node and
      other_rank_clusters_.find(node) == other_rank_clusters_.end()
    ) {
      auto const& [rank_working_bytes, clusters] = rank_summary;
      other_rank_clusters_[node] = clusters;
      other_rank_working_bytes_[node] = rank_working_bytes;
    }
  }

  for (auto&& elm : msg->getNodeInfo()) {
    if (new_load_info_.find(elm.first) == new_load_info_.end()) {
      new_load_info_[elm.first] = elm.second;

      if (isUnderloaded(elm.second.load)) {
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
  case CMFTypeEnum::NormByMax:
  case CMFTypeEnum::NormByMaxExcludeIneligible:
    {
      double l_max = 0.0;
      for (auto&& pe : under) {
        auto iter = load_info_.find(pe);
        vtAssert(iter != load_info_.end(), "Node must be in load_info_");
        auto load = iter->second.load;
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

    auto load = iter->second.load;
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
    if (isUnderloaded(elm.second.load)) {
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
      this_new_load_, elm.second.load, load_to_accommodate, target_max_load_
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
  [[maybe_unused]] LoadType size, ElementLoadType& load,
  std::set<ObjIDType> const& available
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
        if (obj_load >= over_avg && obj_load < single_obj_load) {
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
        auto& selected_load = load_iter->second.load;

	// Check if object is migratable and evaluate criterion for proposed transfer
        bool is_migratable = obj_id.isMigratable();
        bool eval = Criterion(criterion_)(
          this_new_load_, selected_load, obj_load, target_max_load_
        );
        vt_debug_print(
          verbose, temperedlb,
          "TemperedLB::originalTransfer: trial={}, iter={}, under.size()={}, "
          "selected_node={}, selected_load={:e}, obj_id={:x}, home={}, "
          "is_migratable()={}, obj_load={}, target_max_load={}, "
          "this_new_load_={}, criterion={}\n",
          trial_,
          iter_,
          under.size(),
          selected_node,
          selected_load,
          obj_id.id,
          obj_id.getHomeNode(),
          is_migratable,
          obj_load,
          target_max_load_,
          this_new_load_,
          eval
        );

	// Decide about proposed migration based on criterion evaluation
        if (is_migratable and eval) {
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
        n_rejected, n_transfers, 0
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

  if (shared_id != no_shared_id) {
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

double TemperedLB::loadTransferCriterion(
  double before_w_src, double before_w_dst, double after_w_src,
  double after_w_dst
) {
  // Compute maximum work of original arrangement
  auto const w_max_0 = std::max(before_w_src, before_w_dst);

  // Compute maximum work of arrangement after proposed load transfer
  auto const w_max_new = std::max(after_w_src, after_w_dst);

  // Return criterion value
  return w_max_0 - w_max_new;
}

void TemperedLB::considerSwapsAfterLock(MsgSharedPtr<LockedInfoMsg> msg) {
  consider_swaps_counter_++;
  is_swapping_ = true;
  is_locked_ = true;

  vt_debug_print(
    verbose, temperedlb,
    "considerSwapsAfterLock: consider_swaps_counter_={} start\n",
    consider_swaps_counter_
  );

  auto const this_node = theContext()->getNode();

  NodeInfo this_info{
    this_new_load_, this_new_work_,
    this_new_breakdown_.inter_send_vol, this_new_breakdown_.inter_recv_vol,
    this_new_breakdown_.intra_send_vol, this_new_breakdown_.intra_recv_vol,
    this_new_breakdown_.shared_vol
  };

  auto criterion = [&,this](
    auto try_rank, auto const& try_info, auto try_mem,
    auto try_max_object_working_bytes,
    auto try_max_object_serialized_bytes,
    auto const& src_cluster, auto const& try_cluster
  ) -> double {
    BytesType try_new_mem = try_mem;
    try_new_mem -= try_cluster.bytes;
    try_new_mem += src_cluster.bytes;
    try_new_mem -= try_max_object_working_bytes;
    try_new_mem += std::max(
      try_cluster.max_object_working_bytes_outside,
      src_cluster.max_object_working_bytes
    );
    try_new_mem -= try_max_object_serialized_bytes;
    try_new_mem += std::max(
      try_cluster.max_object_serialized_bytes_outside,
      src_cluster.max_object_serialized_bytes
    );
    try_new_mem -= try_cluster.cluster_footprint;
    try_new_mem += src_cluster.cluster_footprint;

    if (try_new_mem > mem_thresh_) {
      return - epsilon;
    }

    BytesType src_new_mem = current_memory_usage_;
    src_new_mem -= src_cluster.bytes;
    src_new_mem += try_cluster.bytes;
    src_new_mem -= max_object_working_bytes_;
    src_new_mem += std::max(
      src_cluster.max_object_working_bytes_outside,
      try_cluster.max_object_working_bytes
    );
    src_new_mem -= max_object_serialized_bytes_;
    src_new_mem += std::max(
      src_cluster.max_object_serialized_bytes_outside,
      try_cluster.max_object_serialized_bytes
    );
    src_new_mem += try_cluster.cluster_footprint;
    src_new_mem -= src_cluster.cluster_footprint;

    if (src_new_mem > mem_thresh_) {
      return - epsilon;
    }

    double const src_new_work =
      computeWorkAfterClusterSwap(this_node, this_info, src_cluster, try_cluster);
    double const dest_new_work =
      computeWorkAfterClusterSwap(try_rank, try_info, try_cluster, src_cluster);

    // Return load transfer criterion
    return loadTransferCriterion(
      this_new_work_, try_info.work, src_new_work, dest_new_work
    );
  };

  auto const& try_clusters = msg->locked_clusters;
  auto const& try_rank = msg->locked_node;
  auto const& try_total_bytes = msg->locked_bytes;
  auto const& try_max_owm = msg->locked_max_object_working_bytes;
  auto const& try_max_osm = msg->locked_max_object_serialized_bytes;
  auto const& try_info = msg->locked_info;

  double best_c_try = -1.0;
  std::tuple<SharedIDType, SharedIDType> best_swap =
    {no_shared_id, no_shared_id};
  for (auto const& [src_shared_id, src_cluster] : cur_clusters_) {
    // try swapping with empty cluster first
    {
      ClusterInfo empty_cluster;
      double c_try = criterion(
        try_rank, try_info, try_total_bytes, try_max_owm, try_max_osm,
        src_cluster, empty_cluster
      );
      if (c_try >= 0.0) {
        if (c_try > best_c_try) {
          best_c_try = c_try;
          best_swap = std::make_tuple(src_shared_id, no_shared_id);
        }
      }
    }

    for (auto const& [try_shared_id, try_cluster] : try_clusters) {
      double c_try = criterion(
        try_rank, try_info, try_total_bytes, try_max_owm, try_max_osm,
        src_cluster, try_cluster
      );
      vt_debug_print(
        verbose, temperedlb,
        "testing a possible swap (rank {}): {} {} c_try={}\n",
        try_rank, src_shared_id, try_shared_id, c_try
      );
      if (c_try >= 0.0) {
        if (c_try > best_c_try) {
          best_c_try = c_try;
          best_swap = std::make_tuple(src_shared_id, try_shared_id);
        }
      }
    }
  }

  if (best_c_try >= 0) {
    // FIXME C++20: use structured binding
    auto const src_shared_id = std::get<0>(best_swap);
    auto const try_shared_id = std::get<1>(best_swap);

    vt_debug_print(
      normal, temperedlb,
      "best_c_try={}, swapping {} for {} on rank ={}\n",
      best_c_try, src_shared_id, try_shared_id, try_rank
    );

    // FIXME C++20: use structured binding
    auto const& give_data = removeClusterToSend(src_shared_id);
    auto const& give_objs = std::get<0>(give_data);
    auto const& give_obj_shared_block = std::get<1>(give_data);
    auto const& give_shared_blocks_size = std::get<2>(give_data);
    auto const& give_obj_working_bytes = std::get<3>(give_data);

    runInEpochRooted("giveCluster", [&]{
      vt_debug_print(
        verbose, temperedlb,
        "considerSwapsAfterLock: giveCluster swapping {} for {}, epoch={:x}\n",
        src_shared_id, try_shared_id, theMsg()->getEpoch()
      );
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
    this_new_breakdown_ = computeWorkBreakdown(this_node, cur_objs_);
    this_new_work_ = this_new_breakdown_.work;
    computeMemoryUsage();

    vt_debug_print(
      normal, temperedlb,
      "best_c_try={}, swap completed with rank={}\n",
      best_c_try, try_rank
    );
  }

  proxy_[try_rank].template send<&TemperedLB::releaseLock>();

  vt_debug_print(
    verbose, temperedlb,
    "considerSwapsAfterLock: consider_swaps_counter_={} finish\n",
    consider_swaps_counter_
  );

  is_swapping_ = false;
  is_locked_ = false;
  consider_swaps_counter_--;

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
  auto const this_node = theContext()->getNode();

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

  if (take_cluster != no_shared_id) {
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
      no_shared_id
    );
  }

  computeClusterSummary();
  this_new_breakdown_ = computeWorkBreakdown(this_node, cur_objs_);
  this_new_work_ = this_new_breakdown_.work;
  computeMemoryUsage();

  vt_debug_print(
    normal, temperedlb,
    "giveCluster: from_rank={}, epoch={:x} total memory usage={}, shared blocks here={}, "
    "memory_threshold={}, give_cluster={}, take_cluster={}\n",
    from_rank, theMsg()->getEpoch(),
    computeMemoryUsage(),
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
    "lockObtained: is_locked_={}, is_swapping_={}, locking_rank_={}, msg->locked_node={}, is_swapping={}\n",
    is_locked_, is_swapping_, locking_rank_, msg->locked_node, is_swapping_
  );

  auto cur_epoch = theMsg()->getEpoch();
  theTerm()->produce(cur_epoch);

  auto action = [this, msg, cur_epoch]{
    theMsg()->pushEpoch(cur_epoch);
    considerSwapsAfterLock(msg);
    theMsg()->popEpoch(cur_epoch);
    theTerm()->consume(cur_epoch);
  };

  if (is_locked_ && locking_rank_ <= msg->locked_node) {
    proxy_[msg->locked_node].template send<&TemperedLB::releaseLock>();
    theTerm()->consume(cur_epoch);
    try_locks_.emplace(msg->locked_node, msg->locked_c_try, 1);
    //pending_actions_.push_back(action);
  } else if (is_locked_) {
    pending_actions_.push_back(action);
  } else if (is_swapping_) {
    pending_actions_.push_back(action);
  } else {
    vt_debug_print(
      normal, temperedlb,
      "lockObtained: running action immediately\n"
    );

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
        "satisfyLockRequest: node={}, c_try={}, forced_release={}\n",
	tl.requesting_node, tl.c_try, tl.forced_release
      );
    }

    auto iter = try_locks_.begin();
    auto lock = *iter;
    try_locks_.erase(iter);

    if (lock.forced_release) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      lock.forced_release = false;
      try_locks_.insert(lock);
      return;
    }

    auto const this_node = theContext()->getNode();

    vt_debug_print(
      normal, temperedlb,
      "satisfyLockRequest: locked obtained for node={}\n",
      lock.requesting_node
    );

    NodeInfo this_info{
      this_new_load_, this_new_work_,
      this_new_breakdown_.inter_send_vol, this_new_breakdown_.inter_recv_vol,
      this_new_breakdown_.intra_send_vol, this_new_breakdown_.intra_recv_vol,
      this_new_breakdown_.shared_vol
    };

    proxy_[lock.requesting_node].template send<&TemperedLB::lockObtained>(
      this_node, cur_clusters_, current_memory_usage_,
      max_object_working_bytes_, max_object_serialized_bytes_,
      lock.c_try, this_info
    );

    is_locked_ = true;
    locking_rank_ = lock.requesting_node;
  }
}

void TemperedLB::swapClusters() {
  n_transfers_swap_ = 0;

  auto const this_node = theContext()->getNode();

  NodeInfo this_info{
    this_new_load_, this_new_work_,
    this_new_breakdown_.inter_send_vol, this_new_breakdown_.inter_recv_vol,
    this_new_breakdown_.intra_send_vol, this_new_breakdown_.intra_recv_vol,
    this_new_breakdown_.shared_vol
  };

  auto lazy_epoch = theTerm()->makeEpochCollective("TemperedLB: swapClusters");
  theTerm()->pushEpoch(lazy_epoch);

  auto criterion = [&,this](
    auto try_rank, auto try_mem, auto const& src_cluster, auto const& try_cluster
  ) -> double {

    // Necessary but not sufficient check regarding memory bounds
    if (try_mem - try_cluster.bytes + src_cluster.bytes > mem_thresh_) {
      return - epsilon;
    }

    auto const src_mem = current_memory_usage_;
    if (src_mem + try_cluster.bytes - src_cluster.bytes > mem_thresh_) {
      return - epsilon;
    }

    auto const& try_info = load_info_.find(try_rank)->second;

    double const src_new_work =
      computeWorkAfterClusterSwap(this_node, this_info, src_cluster, try_cluster);
    double const dest_new_work =
      computeWorkAfterClusterSwap(try_rank, try_info, try_cluster, src_cluster);

    // Return load transfer criterion
    return loadTransferCriterion(
      this_new_work_, try_info.work, src_new_work, dest_new_work
    );
  };

  // Identify and message beneficial cluster swaps
  for (auto const& [try_rank, try_clusters] : other_rank_clusters_) {
    bool found_potential_good_swap = false;

    // Approximate the memory usage on the target
    BytesType try_mem =
      other_rank_working_bytes_.find(try_rank)->second;
    for (auto const& [try_shared_id, try_cluster] : try_clusters) {
      try_mem += try_cluster.bytes;
    }

    // Iterate over source clusters
    for (auto const& [src_shared_id, src_cluster] : cur_clusters_) {
      // Compute approximation swap criterion for empty cluster "swap" case
      {
        ClusterInfo empty_cluster;
        double c_try = criterion(try_rank, try_mem, src_cluster, empty_cluster);
        if (c_try >= 0.0) {
	  // Try to obtain lock for feasible swap
          found_potential_good_swap = true;
          proxy_[try_rank].template send<&TemperedLB::tryLock>(this_node, c_try);
          break;
        }
      }

      // Iterate over target clusters
      for (auto const& [try_shared_id, try_cluster] : try_clusters) {
	// Decide whether swap is beneficial
        double c_try = criterion(try_rank, try_mem, src_cluster, try_cluster);
        if (c_try >= 0.0) {
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
  if (theConfig()->vt_debug_temperedlb) {
    int n_rejected = 0;
    auto remote_block_count = getRemoteBlockCountHere();
    runInEpochCollective("TemperedLB::swapClusters -> compute rejection", [=] {
      proxy_.allreduce<&TemperedLB::rejectionStatsHandler, collective::PlusOp>(
        n_rejected, n_transfers_swap_, remote_block_count
      );
    });
  }
}

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
