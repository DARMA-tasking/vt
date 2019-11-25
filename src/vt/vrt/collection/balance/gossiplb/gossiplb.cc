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
#include "vt/context/context.h"

#include <cstdint>
#include <random>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <set>

namespace vt { namespace vrt { namespace collection { namespace lb {

void GossipLB::init(objgroup::proxy::Proxy<GossipLB> in_proxy) {
  proxy = in_proxy;
}

void GossipLB::inputParams(balance::SpecEntry* spec) {
}

bool GossipLB::isUnderloaded(LoadType load) const {
  auto const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);
  return load < avg * gossip_threshold;
}

bool GossipLB::isOverloaded(LoadType load) const {
  auto const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);
  return load > avg * gossip_threshold;
}

void GossipLB::runLB() {
  bool should_lb = false;

  auto const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);
  auto const max  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::max);
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
    doLBStages();
  } else {
    migrationDone();
  }
}

void GossipLB::doLBStages() {
  for (iter_ = 0; iter_ < num_iters_; iter_++) {
    bool first_iter = iter_ == 0;

    debug_print(
      gossiplb, node,
      "GossipLB::doLBStages: running iter_={}, num_iters_={}\n",
      iter_, num_iters_
    );

    if (first_iter) {
      // Copy this node's object assignments to a local, mutable copy
      cur_objs = *load_data;
      this_new_load = this_load;
    } else {
      // Clear out data structures from previous iteration
      selected_.clear();
      underloaded_.clear();
      load_info_.clear();
      k_cur = 0;
      is_overloaded_ = is_underloaded_ = false;
    }

    if (isOverloaded(this_new_load)) {
      is_overloaded_ = true;
    } else if (isUnderloaded(this_new_load)) {
      is_underloaded_ = true;
    }

    inform();
    decide();
  }

  // Concretize lazy migrations by invoking the BaseLB object migration on new
  // object node assignments
  thunkMigrations();
}

void GossipLB::inform() {
  debug_print(
    gossiplb, node,
    "GossipLB::inform: starting inform phase: k_max={}, k_cur={}\n",
    k_max, k_cur
  );

  vtAssert(k_max > 0, "Number of rounds (k) must be greater than zero");

  auto const this_node = theContext()->getNode();
  if (is_underloaded_) {
    underloaded_.insert(this_node);
  }

  debug_print(
    gossiplb, node,
    "GossipLB::inform: starting inform phase: k_max={}, k_cur={}, "
    "is_underloaded={}, is_overloaded={}\n",
    k_max, k_cur, is_underloaded_, is_overloaded_
  );

  bool inform_done = false;
  auto propagate_epoch = theTerm()->makeEpochCollective();
  theTerm()->addAction(propagate_epoch, [&inform_done] { inform_done = true; });

  // Underloaded start the round
  if (is_underloaded_) {
    propagateRound(propagate_epoch);
  }

  theTerm()->finishedEpoch(propagate_epoch);

  while (not inform_done) {
    vt::runScheduler();
  }

  debug_print(
    gossiplb, node,
    "GossipLB::inform: finished inform phase: k_max={}, k_cur={}\n",
    k_max, k_cur
  );
}

void GossipLB::propagateRound(EpochType epoch) {
  debug_print(
    gossiplb, node,
    "GossipLB::propagateRound: k_max={}, k_cur={}\n",
    k_max, k_cur
  );

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  std::uniform_int_distribution<NodeType> dist(0, num_nodes - 1);
  std::mt19937 gen(seed());

  auto& selected = selected_;
  selected = underloaded_;

  auto const fanout = std::min(f, static_cast<decltype(f)>(num_nodes - 1));

  debug_print(
    gossiplb, node,
    "GossipLB::propagateRound: k_max={}, k_cur={}, selected.size()={}, fanout={}\n",
    k_max, k_cur, selected.size(), fanout
  );

  for (int i = 0; i < fanout; i++) {
    // This implies full knowledge of all processors
    if (selected.size() >= num_nodes - 1) {
      return;
    }

    // First, randomly select a node
    NodeType random_node = uninitialized_destination;

    // Keep generating until we have a unique node for this round
    do {
      random_node = dist(gen);
    } while (
      selected.find(random_node) != selected.end() or
      random_node == this_node
    );

    debug_print(
      gossiplb, node,
      "GossipLB::propagateRound: k_max={}, k_cur={}, sending={}\n",
      k_max, k_cur, random_node
    );

    // Send message with load
    auto msg = makeMessage<GossipMsg>(this_node, load_info_);
    if (epoch != no_epoch) {
      envelopeSetEpoch(msg->env, epoch);
    }
    msg->addNodeLoad(this_node, this_load);
    proxy[random_node].send<GossipMsg, &GossipLB::propagateIncoming>(msg.get());
  }
}

void GossipLB::propagateIncoming(GossipMsg* msg) {
  auto const from_node = msg->getFromNode();

  debug_print(
    gossiplb, node,
    "GossipLB::propagateIncoming: k_max={}, k_cur={}, from_node={}, "
    "load info size={}\n",
    k_max, k_cur, from_node, msg->getNodeLoad().size()
  );

  for (auto&& elm : msg->getNodeLoad()) {
    if (load_info_.find(elm.first) == load_info_.end()) {
      load_info_[elm.first] = elm.second;

      if (isUnderloaded(elm.first)) {
        underloaded_.insert(elm.first);
      }
    }
  }

  if (k_cur == k_max - 1) {
    // nothing to do but wait for termination to be detected
  } else {
    // send out another round
    propagateRound();
    k_cur++;
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
  std::mt19937 gen(seed());

  NodeType selected_node = uninitialized_destination;

  // Pick from the CMF
  auto const u = dist(gen);
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
    if (isUnderloaded(elm.first)) {
      under.push_back(elm.first);
    }
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
  auto lazy_epoch = theTerm()->makeEpochCollective();
  theTerm()->addAction(lazy_epoch, [&decide_done] { decide_done = true; });

  if (is_overloaded_) {
    std::vector<NodeType> under = makeUnderloaded();
    auto cmf = createCMF(under);
    std::unordered_map<NodeType, ObjsType> migrate_objs;

    if (under.size() > 0) {
      // Iterate through all the objects
      for (auto iter = cur_objs.begin(); iter != cur_objs.end(); ) {
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

        debug_print(
          gossiplb, node,
          "GossipLB::decide: under.size()={}, selected_node={}, selected_load={},"
          "load_info_.size()={}, obj_id={:x}, obj_load={}, avg={}, "
          "!(selected_load + obj_load > avg)=!({} + {} > {})={}\n",
          under.size(), selected_node, selected_load, load_info_.size(),
          obj_id, obj_load, avg, selected_load, obj_load, avg,
          not (selected_load + obj_load > avg)
        );

        if (not (selected_load + obj_load > avg)) {
          migrate_objs[selected_node].emplace(std::make_tuple(obj_id, obj_load));

          this_new_load -= obj_load;
          selected_load += obj_load;

          iter = cur_objs.erase(iter);
        } else {
          iter++;
        }

        if (not (this_new_load > avg)) {
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

  while (not decide_done) {
    vt::runScheduler();
  }
}

void GossipLB::thunkMigrations() {
  debug_print(
    gossiplb, node,
    "thunkMigrations, total num_objs={}\n",
    cur_objs.size()
  );

  startMigrationCollective();

  auto this_node = theContext()->getNode();
  for (auto elm : cur_objs) {
    auto obj = elm.first;
    migrateObjectTo(obj, this_node);
  }

  finishMigrationCollective();
}

void GossipLB::inLazyMigrations(balance::LazyMigrationMsg* msg) {
  auto const& incoming_objs = msg->getObjSet();
  for (auto& obj : incoming_objs) {
    auto iter = cur_objs.find(obj.first);
    vtAssert(iter == cur_objs.end(), "Incoming object should not exist");
    cur_objs.insert(obj);
  }
}

void GossipLB::lazyMigrateObjsTo(
  EpochType epoch, NodeType node, ObjsType const& objs
) {
  using LazyMsg = balance::LazyMigrationMsg;
  auto msg = makeMessage<LazyMsg>(node, objs);
  envelopeSetEpoch(msg, epoch);
  proxy[node].send<LazyMsg, &GossipLB::inLazyMigrations>(msg);
}

void GossipLB::migrate() {
  vtAssertExpr(false);
}

}}}} /* end namespace vt::vrt::collection::lb */
