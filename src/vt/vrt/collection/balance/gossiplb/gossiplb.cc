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
    lb, node,
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
    lb, node,
    "GossipLB::inform: starting inform phase: k_max={}, k_cur={}\n",
    k_max, k_cur
  );

  vtAssert(k_max > 0, "Number of rounds (k) must be greater than zero");

  auto const this_node = theContext()->getNode();
  if (is_underloaded_) {
    underloaded_.insert(this_node);
  }
  if (is_overloaded_) {
    underloaded_.insert(this_node);
  }

  bool inform_done = false;
  propagate_epoch_ = theTerm()->makeEpochCollective();

  // Underloaded start the round
  if (is_underloaded_) {
    // Start the round
    propagateRound();
  }

  theTerm()->addAction(propagate_epoch_, [&inform_done] { inform_done = true; });
  theTerm()->finishedEpoch(propagate_epoch_);

  while (not inform_done) {
    vt::runScheduler();
  }

  debug_print(
    lb, node,
    "GossipLB::inform: finished inform phase: k_max={}, k_cur={}\n",
    k_max, k_cur
  );
}

void GossipLB::propagateRound() {
  debug_print(
    lb, node,
    "GossipLB::propagateRound: k_max={}, k_cur={}\n",
    k_max, k_cur
  );

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  std::uniform_int_distribution<NodeType> dist(0, num_nodes - 1);
  std::mt19937 gen(seed());

  auto& selected = selected_;

  if (gossip_prune == GossipInformPrune::PRUNE_UNDER) {
    selected = underloaded_;
  }
  if (gossip_prune == GossipInformPrune::PRUNE_OVER) {
    selected = overloaded_;
  }

  auto const fanout = std::min(f, static_cast<decltype(f)>(num_nodes - 1));

  // This implies full knowledge of all processors
  if (selected.size() >= num_nodes - 1) {
    return;
  }

  for (int i = 0; i < fanout; i++) {
    // First, randomly select a node
    NodeType random_node = uninitialized_destination;

    // Keep generating until we have a unique node for this round
    do {
      random_node = dist(gen);
    } while (
      selected.find(random_node) != selected.end() and
      random_node != this_node
    );

    // Add to selected set
    selected.insert(random_node);

    debug_print(
      lb, node,
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
    lb, node,
    "GossipLB::propagateIncoming: k_max={}, k_cur={}, from_node={}, "
    "load info size={}\n",
    k_max, k_cur, from_node, msg->getNodeLoad().size()
  );

  for (auto&& elm : msg->getNodeLoad()) {
    if (load_info_.find(elm.first) == load_info_.end()) {
      load_info_[elm.first] = elm.second;

      if (gossip_prune == GossipInformPrune::PRUNE_ALL) {
        selected_.insert(elm.first);
      } else if (gossip_prune == GossipInformPrune::PRUNE_UNDER) {
        if (isUnderloaded(elm.first)) {
          underloaded_.insert(elm.first);
        }
      } else if (gossip_prune == GossipInformPrune::PRUNE_OVER) {
        if (isOverloaded(elm.first)) {
          overloaded_.insert(elm.first);
        }
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

void GossipLB::decide() {
  double const avg  = stats.at(lb::Statistic::P_l).at(lb::StatisticQuantity::avg);

  if (is_overloaded_) {
    std::vector<NodeType> under = {};
    for (auto&& elm : load_info_) {
      if (isUnderloaded(elm.first)) {
        under.push_back(elm.first);
      }
    }

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

    // Create the distribution
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::mt19937 gen(seed());

    NodeType selected_node = uninitialized_destination;

    // Pick from the CMF
    auto const u = dist(gen);
    int i = 0;
    for (auto&& x : cmf) {
      if (x >= u) {
        selected_node = under[i];
        break;
      }
      i++;
    }

    // DEBUGGING
    debug_print(
      lb, node,
      "GossipLB::decide: under.size()={}, selected_node={}\n",
      under.size(), selected_node
    );
    i = 0;
    for (auto&& elm : under) {
      debug_print(
        lb, node,
        "\t GossipLB::decide: under[{}]={}\n", i, under[i]
      );
      i++;
    }

  } else {
    // do nothing (underloaded-based algorithm), waits to get work from
    // overloaded nodes
  }
}

void GossipLB::migrate() {
  vtAssertExpr(false);
}

}}}} /* end namespace vt::vrt::collection::lb */
