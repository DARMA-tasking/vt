/*
//@HEADER
// ************************************************************************
//
//                          term_epoch.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "vt/transport.h"

#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static EpochType cur_epoch = no_epoch;

using TTLType = int32_t;

static constexpr EpochType const max_epochs = 5;
static constexpr TTLType const max_life_time = 5;
static constexpr TTLType const no_ttl = -1000;
static constexpr int32_t const max_msgs_per_node = 5;

static bool use_epoch = true;

struct PropagateMsg : vt::Message {
  NodeType from = uninitialized_destination;
  TTLType life_time = no_ttl;

  PropagateMsg(NodeType const& in_from)
    : Message(), from(in_from), life_time(drand48() * max_life_time)
  { }

  PropagateMsg(NodeType const& in_from, TTLType const& in_ttl)
    : Message(), from(in_from), life_time(in_ttl)
  { }
};

static void sendMsgEpoch(EpochType const& epoch, TTLType const& ttl = no_ttl);

static void propagate_handler(PropagateMsg* msg) {
  EpochType const epoch = envelopeGetEpoch(msg->env);
  fmt::print(
    "{}: propagate_handler: msg={}, epoch={:x}\n", my_node, print_ptr(msg), epoch
  );
  if (msg->life_time > 0) {
    sendMsgEpoch(epoch, msg->life_time);
  }
}

static void sendMsgEpoch(EpochType const& epoch, TTLType const& ttl) {
  NodeType const random_node = drand48() * num_nodes;
  PropagateMsg* msg = nullptr;

  if (ttl == no_ttl) {
    // generate a random TLL for the new msg
    msg = makeSharedMessage<PropagateMsg>(my_node);
  } else {
    msg = makeSharedMessage<PropagateMsg>(my_node, ttl - 1);
  }

  if (epoch != no_epoch) {
    theMsg()->setEpochMessage(msg, epoch);
  }

  fmt::print("{}: sending msg: epoch={:x}, ttl={}\n", my_node, epoch, msg->life_time);

  theMsg()->sendMsg<PropagateMsg, propagate_handler>(random_node, msg);

  fmt::print(
    "{}: sendMsgEpoch: epoch={:x}, node={}, ttl-{}\n",
    my_node, epoch, random_node, ttl
  );
}

static void sendStartEpoch(EpochType const& epoch) {
  int32_t const num_msgs = drand48() * max_msgs_per_node;
  for (int i = 0; i < num_msgs; i++) {
    sendMsgEpoch(epoch);
  }
}

static void next_epoch() {
  fmt::print("{}: cur_epoch={:x}\n", my_node, cur_epoch);

  if (use_epoch) {

    if (cur_epoch + 1 < max_epochs) {
      cur_epoch = theTerm()->newEpoch();

      fmt::print("{}: new cur_epoch={:x}\n", my_node, cur_epoch);

      sendStartEpoch(cur_epoch);

      theTerm()->addAction(cur_epoch, []{
        fmt::print("{}: running attached action: cur_epoch={:x}\n", my_node, cur_epoch);
        next_epoch();
      });
      theTerm()->finishedEpoch(cur_epoch);
    }
  } else {
    sendStartEpoch(no_epoch);
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (argc > 1) {
    use_epoch = atoi(argv[1]) == 1 ? true : false;
  }

  fmt::print("{}:use_epoch={}\n", my_node, print_bool(use_epoch));

  if (num_nodes == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  next_epoch();

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
