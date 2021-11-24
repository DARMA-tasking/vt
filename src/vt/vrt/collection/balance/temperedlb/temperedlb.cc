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
#include "vt/vrt/collection/balance/baselb/baselb.h"
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
#include <set>

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
  Approach used to track rounds in the information propagatation step. Options
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
  using a driver that feeds the load balancer object loads read from vt stats
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
  If the final iteration of a trial has a worse imbalance than any earier
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
  };
  return keys_help;
}

void TemperedLB::inputParams(balance::SpecEntry* spec) {
  auto keys_help = getInputKeysWithHelp();

  std::vector<std::string> allowed;
  for (auto&& elm : keys_help) {
    allowed.push_back(elm.first);
  }
  spec->checkAllowedKeys(allowed);

  // the following options interact with each other, so we need to know
  // which were defaulted and which were explicitly specified
  auto params = spec->getParams();
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
  knowledge_ = knowledge_converter_.getFromSpec(spec, knowledge_);

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
      f_ = spec->getOrDefault<int32_t>("fanout", f_);
      k_max_ = static_cast<uint8_t>(
        std::ceil(std::log(num_nodes)/std::log(f_))
      );
    } else if (specified_rounds) {
      // set the fanout based on the chosen rounds: f=p^(1/k)
      k_max_ = spec->getOrDefault<int32_t>("rounds", k_max_);
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
    f_     = spec->getOrDefault<int32_t>("fanout", f_);
    k_max_ = spec->getOrDefault<int32_t>("rounds", k_max_);
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

  num_iters_     = spec->getOrDefault<int32_t>("iters", num_iters_);
  num_trials_    = spec->getOrDefault<int32_t>("trials", num_trials_);

  deterministic_ = spec->getOrDefault<bool>("deterministic", deterministic_);
  rollback_      = spec->getOrDefault<bool>("rollback", rollback_);
  target_pole_   = spec->getOrDefault<bool>("targetpole", target_pole_);

  balance::LBArgsEnumConverter<CriterionEnum> criterion_converter_(
    "criterion", "CriterionEnum", {
      {CriterionEnum::Grapevine,         "Grapevine"},
      {CriterionEnum::ModifiedGrapevine, "ModifiedGrapevine"}
    }
  );
  criterion_ = criterion_converter_.getFromSpec(spec, criterion_);

  balance::LBArgsEnumConverter<InformTypeEnum> inform_type_converter_(
    "inform", "InformTypeEnum", {
      {InformTypeEnum::SyncInform,  "SyncInform"},
      {InformTypeEnum::AsyncInform, "AsyncInform"}
    }
  );
  inform_type_ = inform_type_converter_.getFromSpec(spec, inform_type_);

  balance::LBArgsEnumConverter<ObjectOrderEnum> obj_ordering_converter_(
    "ordering", "ObjectOrderEnum", {
      {ObjectOrderEnum::Arbitrary,        "Arbitrary"},
      {ObjectOrderEnum::ElmID,            "ElmID"},
      {ObjectOrderEnum::FewestMigrations, "FewestMigrations"},
      {ObjectOrderEnum::SmallObjects,     "SmallObjects"},
      {ObjectOrderEnum::LargestObjects,   "LargestObjects"}
    }
  );
  obj_ordering_ = obj_ordering_converter_.getFromSpec(spec, obj_ordering_);

  balance::LBArgsEnumConverter<CMFTypeEnum> cmf_type_converter_(
    "cmf", "CMFTypeEnum", {
      {CMFTypeEnum::Original,                   "Original"},
      {CMFTypeEnum::NormByMax,                  "NormByMax"},
      {CMFTypeEnum::NormBySelf,                 "NormBySelf"},
      {CMFTypeEnum::NormByMaxExcludeIneligible, "NormByMaxExcludeIneligible"}
    }
  );
  cmf_type_ = cmf_type_converter_.getFromSpec(spec, cmf_type_);

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
      obj_ordering_converter_.getString(obj_ordering_),
      cmf_type_converter_.getString(cmf_type_), rollback_, target_pole_
    );
  }
}

void TemperedLB::runLB(TimeType total_load) {
  bool should_lb = false;

  this_load = total_load;
  stats = *getStats();

  auto const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);
  auto const max  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::max);
  auto const pole = stats.at(lb::Statistic::O_l).at(lb::StatisticQuantity::max);
  auto const imb  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::imb);
  auto const load = this_load;

  if (target_pole_) {
    // we can't get the processor max lower than the max object load, so
    // modify the algorithm to define overloaded as exceeding the max
    // object load instead of the processor average load
    target_max_load_ = (pole > avg ? pole : avg);
  } else {
    target_max_load_ = avg;
  }

  if (avg > 0.0000000001) {
    should_lb = max > run_temperedlb_tolerance * target_max_load_;
  }

  if (theContext()->getNode() == 0) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::runLB: avg={}, max={}, pole={}, imb={}, load={}, should_lb={}\n",
      avg, max, pole, imb, load, should_lb
    );
  }

  if (should_lb) {
    doLBStages(imb);
  }
}

void TemperedLB::doLBStages(TimeType start_imb) {
  decltype(this->cur_objs_) best_objs;
  LoadType best_load = 0;
  TimeType best_imb = start_imb + 10;
  uint16_t best_trial = 0;

  auto this_node = theContext()->getNode();

  for (trial_ = 0; trial_ < num_trials_; ++trial_) {
    // Clear out data structures
    selected_.clear();
    underloaded_.clear();
    load_info_.clear();
    is_overloaded_ = is_underloaded_ = false;

    TimeType best_imb_this_trial = start_imb + 10;

    for (iter_ = 0; iter_ < num_iters_; iter_++) {
      bool first_iter = iter_ == 0;

      if (first_iter) {
        // Copy this node's object assignments to a local, mutable copy
        cur_objs_.clear();
        for (auto obj : *load_model_)
          cur_objs_[obj] = load_model_->getWork(obj, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE});
        this_new_load_ = this_load;
      } else {
        // Clear out data structures from previous iteration
        selected_.clear();
        underloaded_.clear();
        load_info_.clear();
        is_overloaded_ = is_underloaded_ = false;
      }

      vt_debug_print(
        normal, temperedlb,
        "TemperedLB::doLBStages: (before) running trial={}, iter={}, "
        "num_iters={}, load={}, new_load={}\n",
        trial_, iter_, num_iters_, this_load, this_new_load_
      );

      if (isOverloaded(this_new_load_)) {
        is_overloaded_ = true;
      } else if (isUnderloaded(this_new_load_)) {
        is_underloaded_ = true;
      }

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

      decide();

      vt_debug_print(
        verbose, temperedlb,
        "TemperedLB::doLBStages: (after) running trial={}, iter={}, "
        "num_iters={}, load={}, new_load={}\n",
        trial_, iter_, num_iters_, this_load, this_new_load_
      );

      if (rollback_ || theConfig()->vt_debug_temperedlb || (iter_ == num_iters_ - 1)) {
        runInEpochCollective("TemperedLB::doLBStages -> P_l reduce", [=] {
          using ReduceOp = collective::PlusOp<std::vector<balance::LoadData>>;
          auto cb = vt::theCB()->makeBcast<
            TemperedLB, StatsMsgType, &TemperedLB::loadStatsHandler
          >(this->proxy_);
          // Perform the reduction for P_l -> processor load only
          auto msg = makeMessage<StatsMsgType>(
            std::vector<balance::LoadData>{
              {balance::LoadData{Statistic::P_l, this_new_load_}}
            }
          );
          this->proxy_.template reduce<ReduceOp>(msg,cb);
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
      vt_print(
        temperedlb,
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
      vt_print(
        temperedlb,
        "TemperedLB::doLBStages: chose trial={} with imb={:0.4f}\n",
        best_trial, new_imbalance_
      );
    }
  } else if (this_node == 0) {
    vt_print(
      temperedlb,
      "TemperedLB::doLBStages: rejected all trials because they would increase imbalance\n"
    );
  }

  // Concretize lazy migrations by invoking the BaseLB object migration on new
  // object node assignments
  thunkMigrations();
}

void TemperedLB::loadStatsHandler(StatsMsgType* msg) {
  auto in = msg->getConstVal()[0];
  new_imbalance_ = in.I();

  auto this_node = theContext()->getNode();
  if (this_node == 0) {
    vt_debug_print(
      terse, temperedlb,
      "TemperedLB::loadStatsHandler: trial={} iter={} max={:0.2f} min={:0.2f} "
      "avg={:0.2f} pole={:0.2f} imb={:0.4f}\n",
      trial_, iter_, in.max(), in.min(), in.avg(),
      stats.at(lb::Statistic::O_l).at(lb::StatisticQuantity::max),
      in.I()
    );
  }
}

void TemperedLB::rejectionStatsHandler(RejectionMsgType* msg) {
  auto in = msg->getConstVal();

  auto n_rejected = in.n_rejected_;
  auto n_transfers = in.n_transfers_;
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
    trial_, iter_, k_max_, is_underloaded_, is_overloaded_, this_new_load_
  );

  vtAssert(k_max_ > 0, "Number of rounds (k) must be greater than zero");

  auto const this_node = theContext()->getNode();
  if (is_underloaded_) {
    underloaded_.insert(this_node);
  }

  setup_done_ = false;

  auto cb = theCB()->makeBcast<TemperedLB, ReduceMsgType, &TemperedLB::setupDone>(proxy_);
  auto msg = makeMessage<ReduceMsgType>();
  proxy_.reduce(msg.get(), cb);

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

  auto cb = theCB()->makeBcast<TemperedLB, ReduceMsgType, &TemperedLB::setupDone>(proxy_);
  auto msg = makeMessage<ReduceMsgType>();
  proxy_.reduce(msg.get(), cb);

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

void TemperedLB::setupDone(ReduceMsgType* msg) {
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

  auto const fanout = std::min(f_, static_cast<decltype(f_)>(num_nodes - 1));

  vt_debug_print(
    verbose, temperedlb,
    "TemperedLB::propagateRound: trial={}, iter={}, k_max={}, k_cur={}, "
    "selected.size()={}, fanout={}\n",
    trial_, iter_, k_max_, k_cur, selected.size(), fanout
  );

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
      auto msg = makeMessage<LoadMsgSync>(this_node, load_info_);
      if (epoch != no_epoch) {
        envelopeSetEpoch(msg->env, epoch);
      }
      msg->addNodeLoad(this_node, this_new_load_);
      proxy_[random_node].sendMsg<
        LoadMsgSync, &TemperedLB::propagateIncomingSync
      >(msg.get());
    } else {
      auto msg = makeMessage<LoadMsgAsync>(this_node, load_info_, k_cur);
      if (epoch != no_epoch) {
        envelopeSetEpoch(msg->env, epoch);
      }
      msg->addNodeLoad(this_node, this_new_load_);
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
  TimeType load_to_accommodate
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
  std::unordered_map<ObjIDType, TimeType> cur_objs,
  LoadType this_new_load, TimeType target_max_load
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
      vt_debug_print(
        normal, temperedlb,
        "TemperedLB::decide: over_avg={}, single_obj_load={}\n",
        over_avg, cur_objs[ordered_obj_ids[0]]
      );
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
      vt_debug_print(
        normal, temperedlb,
        "TemperedLB::decide: over_avg={}, marginal_obj_load={}\n",
        over_avg, cur_objs[ordered_obj_ids[0]]
      );
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

void TemperedLB::decide() {
  auto lazy_epoch = theTerm()->makeEpochCollective("TemperedLB: decide");

  int n_transfers = 0, n_rejected = 0;

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
          "TemperedLB::decide: selected_node={}, load_info_.size()={}\n",
          selected_node, load_info_.size()
        );

        auto load_iter = load_info_.find(selected_node);
        vtAssert(load_iter != load_info_.end(), "Selected node not found");

        // The load of the node selected
        auto& selected_load = load_iter->second;

        bool eval = Criterion(criterion_)(
          this_new_load_, selected_load, obj_load, target_max_load_
        );

        vt_debug_print(
          verbose, temperedlb,
          "TemperedLB::decide: trial={}, iter={}, under.size()={}, "
          "selected_node={}, selected_load={:e}, obj_id={:x}, home={}, "
          "obj_load={:e}, target_max_load={:e}, this_new_load_={:e}, "
          "criterion={}\n",
          trial_,
          iter_,
          under.size(),
          selected_node,
          selected_load,
          obj_id.id,
          obj_id.home_node,
          obj_load,
          target_max_load_,
          this_new_load_,
          eval
        );

        if (eval) {
          ++n_transfers;
          // transfer the object load in seconds
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
    runInEpochCollective("TemperedLB::decide -> compute rejection", [=] {
      using ReduceOp = collective::PlusOp<balance::RejectionStats>;
      auto cb = vt::theCB()->makeBcast<
        TemperedLB, RejectionMsgType, &TemperedLB::rejectionStatsHandler
      >(this->proxy_);
      auto msg = makeMessage<RejectionMsgType>(n_rejected, n_transfers);
      this->proxy_.template reduce<ReduceOp>(msg,cb);
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

}}}} /* end namespace vt::vrt::collection::lb */
