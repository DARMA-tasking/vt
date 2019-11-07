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

  if (isOverloaded(load)) {
    is_overloaded_ = true;
  } else if (isUnderloaded(load)) {
    is_underloaded_ = true;
  }

  debug_print(
    gossiplb, node,
    "GossipLB::runLB: avg={}, max={}, load={},"
    " overloaded_={}, underloaded_={}, should_lb={}\n",
    avg, max, load, is_overloaded_, is_underloaded_, should_lb
  );

  if (should_lb) {
    this->inform();

    for (auto&& elm : load_info_) {
      vt_print(lb, "inform info: node={}, load={}\n", elm.first, elm.second);
    }

    this->decide();

    startMigrationCollective();
    finishMigrationCollective();
  } else {
    migrationDone();
  }
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

  bool inform_done = false;
  propagate_epoch_ = theTerm()->makeEpochCollective();

  // Underloaded start the round
  if (is_underloaded_) {
    // Start the round
    this->propagateRound();
  }

  theTerm()->addAction(propagate_epoch_, [&inform_done] { inform_done = true; });
  theTerm()->finishedEpoch(propagate_epoch_);

  while (not inform_done) {
    vt::runScheduler();
  }

  debug_print(
    gossiplb, node,
    "GossipLB::inform: finished inform phase: k_max={}, k_cur={}\n",
    k_max, k_cur
  );
}

void GossipLB::propagateRound() {
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
    if (selected.size() == num_nodes) {
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

    // Add to selected set
    selected.insert(random_node);

    debug_print(
      gossiplb, node,
      "GossipLB::propagateRound: k_max={}, k_cur={}, sending={}\n",
      k_max, k_cur, random_node
    );

    // Send message with load
    auto msg = makeMessage<GossipMsg>(this_node, this->load_info_);
    envelopeSetEpoch(msg, propagate_epoch_);
    msg->addNodeLoad(this_node, this->this_load);
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

  if (this->k_cur == this->k_max - 1) {
    // nothing to do but wait for termination to be detected
  } else {
    // send out another round
    this->propagateRound();
    this->k_cur++;
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

// //////////////////////////////////////////////////////////////////
// // DEBUGGING
// ///////////////////////////////////////////////////////////////////
// debug_print(
//   gossiplb, node,
//   "GossipLB::decide: under.size()={}, selected_node={}\n",
//   under.size(), selected_node
// );
// int i = 0;
// for (auto&& elm : under) {
//   debug_print(
//     gossiplb, node,
//     "\t GossipLB::decide: under[{}]={}\n", i, under[i]
//   );
//   i++;
// }
// //////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////


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
  lazy_epoch_ = theTerm()->makeEpochCollective();
  theTerm()->addAction(lazy_epoch_, [&decide_done] { decide_done = true; });

  if (is_overloaded_) {
    std::vector<NodeType> under = makeUnderloaded();
    auto cmf = createCMF(under);
    double this_new_load = this_load;
    std::unordered_map<ObjIDType, TimeType> cur_objs = *load_data;
    std::set<ObjIDType> objs_to_select;
    std::set<ObjIDType> objs_prev_selected;
    std::unordered_map<NodeType, std::vector<ObjIDType>> selected_objects;

    do {
      auto const selected_node = sampleFromCMF(under, cmf);
      vtAssertExpr(load_info_.find(selected_node) != load_info_.end());
      auto& selected_load = load_info_[selected_node];

      objs_to_select.clear();
      objs_prev_selected.clear();
      for (auto&& elm : cur_objs) {
        objs_to_select.insert(elm.first);
      }

      do {

        std::set<ObjIDType> available_objs;
        std::set_difference(
          objs_to_select.begin(), objs_to_select.end(),
          objs_prev_selected.begin(), objs_prev_selected.end(),
          std::inserter(available_objs, available_objs.end())
        );

        if (available_objs.size() == 0) {
          break;
        }

        debug_print(
          lb, node,
          "GossipLB::decide: available.size()={}, objs_to_select={}, "
          "objs_prev_selected={}\n",
          available_objs.size(), objs_to_select.size(), objs_prev_selected.size()
        );

        auto max_obj_size = avg - selected_load;
        auto iter = selectObject(max_obj_size, cur_objs, available_objs);

        vtAssert(iter != cur_objs.end(), "Must have objects to select");

        auto obj_id = iter->first;

        // @todo: for now, convert to milliseconds due to the stats framework all
        // computing in milliseconds; should be converted to seconds along with
        // the rest of the stats framework
        auto obj_load = loadMilli(iter->second);

        objs_prev_selected.insert(obj_id);

        debug_print(
          lb, node,
          "GossipLB::decide: under.size()={}, selected_node={}, selected_load={},"
          "load_info_.size()={}, obj_id={:x}, obj_load={}, avg={}, "
          "!(selected_load + obj_load > avg)=!({} + {} > {})={}\n",
          under.size(), selected_node, selected_load, load_info_.size(),
          obj_id, obj_load, avg, selected_load, obj_load, avg,
          not (selected_load + obj_load > avg)
        );

        if (not (selected_load + obj_load > avg)) {
          selected_objects[selected_node].push_back(obj_id);

          this_new_load -= obj_load;
          selected_load += obj_load;

          cur_objs.erase(iter);
        }

      } while (not (selected_load > avg));

    } while (this_new_load > avg and cur_objs.size() > 0);

    // Send objects to nodes
    for (auto&& migration : selected_objects) {
      auto node = migration.first;
      for (auto&& obj_id : migration.second) {
        this->lazyMigrateObjectTo(obj_id, node);
      }
    }

    this->informLazyMigrations();


  } else {
    // do nothing (underloaded-based algorithm), waits to get work from
    // overloaded nodes
  }

  theTerm()->finishedEpoch(lazy_epoch_);

  while (not decide_done) {
    vt::runScheduler();
  }
}

void GossipLB::inLazyMigrations(balance::LazyMigrationMsg* msg) {

}

void GossipLB::informLazyMigrations() {
  for (auto&& elm : lazy_migrations_) {
    auto const& node = elm.first;
    auto const& objs = elm.second;

    using LazyMsg = balance::LazyMigrationMsg;
    auto msg = makeMessage<LazyMsg>(node, objs);
    envelopeSetEpoch(msg, lazy_epoch_);
    proxy[node].send<LazyMsg, &GossipLB::inLazyMigrations>(msg);
  }
}

void GossipLB::lazyMigrateObjectTo(ObjIDType const obj_id, NodeType const node) {
  lazy_migrations_[node].push_back(obj_id);
}

void GossipLB::migrate() {
  vtAssertExpr(false);
}

}}}} /* end namespace vt::vrt::collection::lb */
