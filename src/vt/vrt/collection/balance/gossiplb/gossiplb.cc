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
#include "vt/context/context.h"

#include <cstdint>
#include <random>
#include <algorithm>
#include <unordered_set>

namespace vt { namespace vrt { namespace collection { namespace lb {

void GossipLB::init(objgroup::proxy::Proxy<GossipLB> in_proxy) {
  proxy = in_proxy;
}

void GossipLB::inputParams(balance::SpecEntry* spec) {
}

void GossipLB::runLB() {
  this->inform();

  for (auto&& elm : load_info_) {
    vt_print(lb, "inform info: node={}, load={}\n", elm.first, elm.second);
  }

}

void GossipLB::inform() {
  debug_print(
    lb, node,
    "GossipLB::inform: starting inform phase: k_max={}, k_cur={}\n",
    k_max, k_cur
  );

  vtAssert(k_max > 0, "Number of rounds (k) must be greater than zero");

  bool inform_done = false;
  propagate_epoch_ = theTerm()->makeEpochCollective();

  // Start the round
  propagateRound();

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

  auto epoch = startMigrationCollective();
  finishMigrationCollective();
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
  std::unordered_set<NodeType> selected = {};

  // Insert this node to inhibit self-send
  selected.insert(this_node);

  auto const fanout = std::min(f, static_cast<decltype(f)>(num_nodes - 1));

  for (int i = 0; i < fanout; i++) {
    // First, randomly select a node
    NodeType random_node = uninitialized_destination;

    // Keep generating until we have a unique node for this round
    do {
      random_node = dist(gen);
    } while (selected.find(random_node) != selected.end());

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
    "GossipLB::propagateIncoming: k_max={}, k_cur={}, from_node={}\n",
    k_max, k_cur, from_node
  );

  for (auto&& elm : msg->getNodeLoad()) {
    this->load_info_[elm.first] = elm.second;
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
  vtAssertExpr(false);
}

void GossipLB::migrate() {
  vtAssertExpr(false);
}

}}}} /* end namespace vt::vrt::collection::lb */
