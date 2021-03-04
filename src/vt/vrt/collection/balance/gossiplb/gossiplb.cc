/*
//@HEADER
// *****************************************************************************
//
//                                 gossiplb.cc
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
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/gossiplb/gossiplb.h"
#include "vt/vrt/collection/balance/gossiplb/gossip_msg.h"
#include "vt/vrt/collection/balance/gossiplb/gossip_constants.h"
#include "vt/vrt/collection/balance/gossiplb/criterion.h"
#include "vt/context/context.h"
#include "vt/pipe/pipe_manager.h"

#include <cstdint>
#include <random>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <set>

namespace vt { namespace vrt { namespace collection { namespace lb {

void GossipLB::init(objgroup::proxy::Proxy<GossipLB> in_proxy) {
  proxy_ = in_proxy;
  auto const this_node = theContext()->getNode();
  gen_propagate_.seed(this_node + 12345);
  gen_sample_.seed(this_node + 54321);
}

bool GossipLB::isUnderloaded(LoadType load) const {
  auto const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);
  return load < avg * gossip_threshold;
}

bool GossipLB::isOverloaded(LoadType load) const {
  auto const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);
  return load > avg * gossip_threshold;
}

void GossipLB::inputParams(balance::SpecEntry* spec) {
  std::vector<std::string> allowed{"f", "k", "i", "c", "trials", "deterministic"};
  spec->checkAllowedKeys(allowed);
  using CriterionEnumUnder = typename std::underlying_type<CriterionEnum>::type;
  using InformTypeEnumUnder = typename std::underlying_type<InformTypeEnum>::type;

  auto default_c = static_cast<CriterionEnumUnder>(criterion_);
  auto default_inform = static_cast<InformTypeEnumUnder>(inform_type_);

  f_             = spec->getOrDefault<int32_t>("f", f_);
  k_max_         = spec->getOrDefault<int32_t>("k", k_max_);
  num_iters_     = spec->getOrDefault<int32_t>("i", num_iters_);
  num_trials_    = spec->getOrDefault<int32_t>("trials", num_trials_);
  deterministic_ = spec->getOrDefault<int32_t>("deterministic", deterministic_);
  int32_t c      = spec->getOrDefault<int32_t>("c", default_c);
  criterion_     = static_cast<CriterionEnum>(c);
  int32_t inf    = spec->getOrDefault<int32_t>("inform", default_inform);
  inform_type_   = static_cast<InformTypeEnum>(inf);

  vtAbortIf(
    inform_type_ == InformTypeEnum::AsyncInform && deterministic_,
    "Asynchronous informs allow race conditions and thus are not deterministic"
  );
}

void GossipLB::runLB() {
  bool should_lb = false;

  auto const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);
  auto const max  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::max);
  auto const imb  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::imb);
  auto const load = this_load;

  if (avg > 0.0000000001) {
    should_lb = max > gossip_tolerance * avg;
  }

  if (theContext()->getNode() == 0) {
    vt_print(
      gossiplb,
      "GossipLB::runLB: avg={}, max={}, load={},"
      " overloaded_={}, underloaded_={}, should_lb={}\n",
      avg, max, load, is_overloaded_, is_underloaded_, should_lb
    );
  }

  if (should_lb) {
    doLBStages(imb);
  } else {
    migrationDone();
  }
}

void GossipLB::doLBStages(TimeType start_imb) {
  decltype(this->cur_objs_) best_objs;
  LoadType best_load = 0;
  TimeType best_imb = start_imb+1;
  uint16_t best_trial = 0;

  auto this_node = theContext()->getNode();

  for (trial_ = 0; trial_ < num_trials_; ++trial_) {
    // Clear out data structures
    selected_.clear();
    underloaded_.clear();
    load_info_.clear();
    k_cur_ = 0;
    is_overloaded_ = is_underloaded_ = false;

    for (iter_ = 0; iter_ < num_iters_; iter_++) {
      bool first_iter = iter_ == 0;

      debug_print(
        gossiplb, node,
        "GossipLB::doLBStages: (before) running trial={}, iter={}, "
        "num_iters={}, load={}, new_load={}\n",
        trial_, iter_, num_iters_, this_load, this_new_load_
      );

      if (first_iter) {
        // Copy this node's object assignments to a local, mutable copy
        cur_objs_.clear();
        for (auto obj : *load_data) {
          cur_objs_[obj.first] = obj.second;
        }
        this_new_load_ = this_load;
      } else {
        // Clear out data structures from previous iteration
        selected_.clear();
        underloaded_.clear();
        load_info_.clear();
        k_cur_ = 0;
        is_overloaded_ = is_underloaded_ = false;
      }

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
        vtAbort("GossipLB:: Unsupported inform type");
      }

      decide();

      debug_print(
        gossiplb, node,
        "GossipLB::doLBStages: (after) running trial={}, iter={}, "
        "num_iters={}, load={}, new_load={}\n",
        trial_, iter_, num_iters_, this_load, this_new_load_
      );

      runInEpochCollective([=] {
        using StatsMsgType = balance::ProcStatsMsg;
        using ReduceOp = collective::PlusOp<balance::LoadData>;
        auto cb = vt::theCB()->makeBcast<
          GossipLB, StatsMsgType, &GossipLB::gossipStatsHandler
        >(this->proxy_);
        // Perform the reduction for P_l -> processor load only
        auto msg = makeMessage<StatsMsgType>(Statistic::P_l, this_new_load_);
        this->proxy_.template reduce<ReduceOp>(msg,cb);
      });

      if (this_node == 0) {
        vt_print(
          gossiplb,
          "GossipLB::doLBStages: trial={} iter={} imb={:0.2f}\n",
          trial_, iter_, new_imbalance_
        );
      }
    }

    if (this_node == 0) {
      vt_print(
        gossiplb,
        "GossipLB::doLBStages: trial={} imb={:0.2f}\n",
        trial_, new_imbalance_
      );
    }

    if (new_imbalance_ <= start_imb && new_imbalance_ < best_imb) {
      best_load = this_new_load_;
      best_objs = cur_objs_;
      best_imb = new_imbalance_;
      best_trial = trial_;
    }

    // Clear out for next try or for not migrating by default
    cur_objs_.clear();
    this_new_load_ = this_load;
  }

  if (best_imb <= start_imb) {
    cur_objs_ = best_objs;
    this_load = this_new_load_ = best_load;
    new_imbalance_ = best_imb;

    // Update the load based on new object assignments
    if (this_node == 0) {
      vt_print(
        gossiplb,
        "GossipLB::doLBStages: chose trial={} with imb={:0.2f}\n",
        best_trial, new_imbalance_
      );
    }
  } else if (this_node == 0) {
    vt_print(
      gossiplb,
      "GossipLB::doLBStages: rejected all trials because they would increase imbalance\n"
    );
  }

  // Concretize lazy migrations by invoking the BaseLB object migration on new
  // object node assignments
  thunkMigrations();
}

void GossipLB::gossipStatsHandler(StatsMsgType* msg) {
  auto in = msg->getConstVal();
  new_imbalance_ = in.I();

  auto this_node = theContext()->getNode();
  if (this_node == 0) {
    vt_print(
      gossiplb,
      "GossipLB::gossipStatsHandler: max={:0.2f} min={:0.2f} avg={:0.2f} imb={:0.2f}\n",
      in.max(), in.min(), in.avg(), in.I()
    );
  }
}

void GossipLB::gossipRejectionStatsHandler(GossipRejectionMsgType* msg) {
  auto in = msg->getConstVal();

  auto n_rejected = in.n_rejected_;
  auto n_transfers = in.n_transfers_;
  double rej = static_cast<double>(n_rejected) / static_cast<double>(n_rejected + n_transfers) * 100.0;

  auto this_node = theContext()->getNode();
  if (this_node == 0) {
    vt_print(
      gossiplb,
      "GossipLB::gossipRejectionStatsHandler: n_transfers={} n_rejected={} rejection_rate={:0.1f}%\n",
      n_transfers, n_rejected, rej
    );
  }
}

void GossipLB::informAsync() {
  propagated_k_.assign(k_max_, false);
  uint8_t k_cur_async = 0;

  debug_print(
    gossiplb, node,
    "GossipLB::informAsync: starting inform phase: trial={}, iter={}, "
    "k_max={}, k_cur={}, is_underloaded={}, is_overloaded={}, load={}\n",
    trial_, iter_, k_max_, k_cur_, is_underloaded_, is_overloaded_, this_new_load_
  );

  vtAssert(k_max_ > 0, "Number of rounds (k) must be greater than zero");

  auto const this_node = theContext()->getNode();
  if (is_underloaded_) {
    underloaded_.insert(this_node);
  }

  setup_done_ = false;

  auto cb = theCB()->makeBcast<GossipLB, ReduceMsgType, &GossipLB::setupDone>(proxy_);
  auto msg = makeMessage<ReduceMsgType>();
  proxy_.reduce(msg.get(), cb);

  theSched()->runSchedulerWhile([this]{ return not setup_done_; });

  bool inform_done = false;
  auto propagate_epoch = theTerm()->makeEpochCollective("GossipLB: informAsync");
  theTerm()->addAction(propagate_epoch, [&inform_done] { inform_done = true; });

  // Underloaded start the round
  if (is_underloaded_) {
    propagateRoundAsync(k_cur_async, propagate_epoch);
  }

  theTerm()->finishedEpoch(propagate_epoch);

  theSched()->runSchedulerWhile([&inform_done]{ return not inform_done; });

  if (is_overloaded_) {
    vt_print(
      gossiplb,
      "GossipLB::informAsync: trial={}, iter={}, known underloaded={}\n",
      trial_, iter_, underloaded_.size()
    );
  }

  debug_print(
    gossiplb, node,
    "GossipLB::informAsync: finished inform phase: trial={}, iter={}, "
    "k_max={}, k_cur={}\n",
    trial_, iter_, k_max_, k_cur_
  );
}

void GossipLB::informSync() {
  debug_print(
    gossiplb, node,
    "GossipLB::informSync: starting inform phase: trial={}, iter={}, "
    "k_max={}, k_cur={}, is_underloaded={}, is_overloaded={}, load={}\n",
    trial_, iter_, k_max_, k_cur_, is_underloaded_, is_overloaded_, this_new_load_
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

  auto cb = theCB()->makeBcast<GossipLB, ReduceMsgType, &GossipLB::setupDone>(proxy_);
  auto msg = makeMessage<ReduceMsgType>();
  proxy_.reduce(msg.get(), cb);

  theSched()->runSchedulerWhile([this]{ return not setup_done_; });

  for (; k_cur_ < k_max_; ++k_cur_) {
    auto name = fmt::format("GossipLB: informSync k_cur={}", k_cur_);
    auto propagate_epoch = theTerm()->makeEpochCollective(name);

    // Underloaded start the first round; ranks that received on some round
    // start subsequent rounds
    if (propagate_this_round) {
      propagateRoundSync(propagate_epoch);
    }

    theTerm()->finishedEpoch(propagate_epoch);

    vt::runSchedulerThrough(propagate_epoch);

    propagate_this_round = propagate_next_round_;
    propagate_next_round_ = false;
    underloaded_ = new_underloaded_;
    load_info_ = new_load_info_;
  }

  if (is_overloaded_) {
    vt_print(
      gossiplb,
      "GossipLB::informSync: trial={}, iter={}, known underloaded={}\n",
      trial_, iter_, underloaded_.size()
    );
  }

  debug_print(
    gossiplb, node,
    "GossipLB::informSync: finished inform phase: trial={}, iter={}, "
    "k_max={}, k_cur={}\n",
    trial_, iter_, k_max_, k_cur_
  );
}

void GossipLB::setupDone(ReduceMsgType* msg) {
  setup_done_ = true;
}

void GossipLB::propagateRoundAsync(uint8_t k_cur_async, EpochType epoch) {
  debug_print(
    gossiplb, node,
    "GossipLB::propagateRoundAsync: trial={}, iter={}, k_max={}, k_cur={}\n",
    trial_, iter_, k_max_, k_cur_
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

  debug_print(
    gossiplb, node,
    "GossipLB::propagateRoundAsync: trial={}, iter={}, k_max={}, k_cur={}, "
    "selected.size()={}, fanout={}\n",
    trial_, iter_, k_max_, k_cur_, selected.size(), fanout
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

    debug_print(
      gossiplb, node,
      "GossipLB::propagateRoundAsync: trial={}, iter={}, k_max={}, "
      "k_cur={}, sending={}\n",
      trial_, iter_, k_max_, k_cur_, random_node
    );

    // Send message with load
    auto msg = makeMessage<GossipMsgAsync>(this_node, load_info_, k_cur_async);
    if (epoch != no_epoch) {
      envelopeSetEpoch(msg->env, epoch);
    }
    msg->addNodeLoad(this_node, this_new_load_);
    proxy_[random_node].send<
      GossipMsgAsync, &GossipLB::propagateIncomingAsync
    >(msg.get());
  }
}

void GossipLB::propagateRoundSync(EpochType epoch) {
  debug_print(
    gossiplb, node,
    "GossipLB::propagateRoundSync: trial={}, iter={}, k_max={}, k_cur={}\n",
    trial_, iter_, k_max_, k_cur_
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

  debug_print(
    gossiplb, node,
    "GossipLB::propagateRoundSync: trial={}, iter={}, k_max={}, k_cur={}, "
    "selected.size()={}, fanout={}\n",
    trial_, iter_, k_max_, k_cur_, selected.size(), fanout
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

    debug_print(
      gossiplb, node,
      "GossipLB::propagateRoundSync: k_max_={}, k_cur_={}, sending={}\n",
      k_max_, k_cur_, random_node
    );

    // Send message with load
    auto msg = makeMessage<GossipMsgSync>(this_node, load_info_);
    if (epoch != no_epoch) {
      envelopeSetEpoch(msg->env, epoch);
    }
    msg->addNodeLoad(this_node, this_new_load_);
    proxy_[random_node].send<GossipMsgSync, &GossipLB::propagateIncomingSync>(msg.get());
  }
}

void GossipLB::propagateIncomingAsync(GossipMsgAsync* msg) {
  auto const from_node = msg->getFromNode();
  auto k_cur_async = msg->getRound();

  debug_print(
    gossiplb, node,
    "GossipLB::propagateIncomingAsync: trial={}, iter={}, k_max={}, "
    "k_cur={}, from_node={}, load info size={}\n",
    trial_, iter_, k_max_, k_cur_, from_node, msg->getNodeLoad().size()
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
    propagateRoundAsync(k_cur_async + 1);
  }
}

void GossipLB::propagateIncomingSync(GossipMsgSync* msg) {
  auto const from_node = msg->getFromNode();

  // we collected more info that should be propagated on the next round
  propagate_next_round_ = true;

  debug_print(
    gossiplb, node,
    "GossipLB::propagateIncomingSync: trial={}, iter={}, k_max={}, "
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

std::vector<double> GossipLB::createCMF(NodeSetType const& under) {
  double const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);

  // Build the CMF
  double sum_p = 0.0;
  double inv_l_avg = 1.0 / avg;
  std::vector<double> cmf = {};

  for (auto&& pe : under) {
    auto iter = load_info_.find(pe);
    vtAssert(iter != load_info_.end(), "Node must be in load_info_");

    auto load = iter->second;
    sum_p += 1. - inv_l_avg * load;
    cmf.push_back(sum_p);
  }

  // Normalize the CMF
  for (auto& elm : cmf) {
    elm /= sum_p;
  }

  vtAssertExpr(cmf.size() == under.size());

  return cmf;
}

NodeType GossipLB::sampleFromCMF(
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

std::vector<NodeType> GossipLB::makeUnderloaded() const {
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

GossipLB::ElementLoadType::iterator
GossipLB::selectObject(
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

void GossipLB::decide() {
  double const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);

  bool decide_done = false;
  auto lazy_epoch = theTerm()->makeEpochCollective("GossipLB: decide");
  theTerm()->addAction(lazy_epoch, [&decide_done] { decide_done = true; });

  int n_transfers = 0, n_rejected = 0;

  if (is_overloaded_) {
    std::vector<NodeType> under = makeUnderloaded();
    std::unordered_map<NodeType, ObjsType> migrate_objs;

    if (under.size() > 0) {
      // Iterate through all the objects
      for (auto iter = cur_objs_.begin(); iter != cur_objs_.end(); ) {
        // Rebuild the relaxed underloaded set based on updated load of this node
        under = makeUnderloaded();
        if (under.size() == 0) {
          break;
        }
        // Rebuild the CMF with the new loads taken into account
        auto cmf = createCMF(under);
        // Select a node using the CMF
        auto const selected_node = sampleFromCMF(under, cmf);

        debug_print(
          gossiplb, node,
          "GossipLB::decide: selected_node={}, load_info_.size()\n",
          selected_node, load_info_.size()
        );

        auto load_iter = load_info_.find(selected_node);
        vtAssert(load_iter != load_info_.end(), "Selected node not found");

        // The load of the node selected
        auto& selected_load = load_iter->second;

        //auto max_obj_size = avg - selected_load;
        auto obj_id = iter->first;

        // @todo: for now, convert to milliseconds due to the stats framework all
        // computing in milliseconds; should be converted to seconds along with
        // the rest of the stats framework
        auto obj_load = loadMilli(iter->second);

        bool eval = Criterion(criterion_)(this_new_load_, selected_load, obj_load, avg);

        debug_print(
          gossiplb, node,
          "GossipLB::decide: trial={}, iter={}, under.size()={}, "
          "selected_node={}, selected_load={:e}, obj_id={:x}, obj_load={:e}, "
          "avg={:e}, this_new_load_={:e}, criterion={}\n",
          trial_,
          iter_,
          under.size(),
          selected_node,
          selected_load,
          obj_id,
          obj_load,
          avg,
          this_new_load_,
          eval
        );

        if (eval) {
          ++n_transfers;
          migrate_objs[selected_node][obj_id] = obj_load;

          this_new_load_ -= obj_load;
          selected_load += obj_load;

          iter = cur_objs_.erase(iter);
        } else {
          ++n_rejected;
          iter++;
        }

        if (not (this_new_load_ > avg)) {
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

  theSched()->runSchedulerWhile([&decide_done]{ return not decide_done; });

  runInEpochCollective([=] {
    using ReduceOp = collective::PlusOp<balance::RejectionStats>;
    auto cb = vt::theCB()->makeBcast<
      GossipLB, GossipRejectionMsgType, &GossipLB::gossipRejectionStatsHandler
    >(this->proxy_);
    auto msg = makeMessage<GossipRejectionMsgType>(n_rejected, n_transfers);
    this->proxy_.template reduce<ReduceOp>(msg,cb);
  });
}

void GossipLB::thunkMigrations() {
  debug_print(
    gossiplb, node,
    "thunkMigrations, total num_objs={}\n",
    cur_objs_.size()
  );

  startMigrationCollective();

  auto this_node = theContext()->getNode();
  for (auto elm : cur_objs_) {
    auto obj = elm.first;
    migrateObjectTo(obj, this_node);
  }

  finishMigrationCollective();
}

void GossipLB::inLazyMigrations(balance::LazyMigrationMsg* msg) {
  auto const& incoming_objs = msg->getObjSet();
  for (auto& obj : incoming_objs) {
    auto iter = cur_objs_.find(obj.first);
    vtAssert(iter == cur_objs_.end(), "Incoming object should not exist");
    cur_objs_.insert(obj);
    this_new_load_ += obj.second;
  }
}

void GossipLB::lazyMigrateObjsTo(
  EpochType epoch, NodeType node, ObjsType const& objs
) {
  using LazyMsg = balance::LazyMigrationMsg;
  auto msg = makeMessage<LazyMsg>(node, objs);
  envelopeSetEpoch(msg->env, epoch);
  proxy_[node].send<LazyMsg, &GossipLB::inLazyMigrations>(msg);
}

void GossipLB::migrate() {
  vtAssertExpr(false);
}

}}}} /* end namespace vt::vrt::collection::lb */
