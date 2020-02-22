/*
//@HEADER
// *****************************************************************************
//
//                              broadcast_test.cc
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

#include "vt/transport.h"
#include <cstdlib>

using namespace vt;

#define BCAST_DEBUG 1

static constexpr int32_t const num_bcasts = 4;   // number of intial root bcasts
static constexpr int32_t const bcast_depth = 1; // 10; // number re-broadcasts
static NodeType my_node = uninitialized_destination;
static int32_t bcast_count = 0;

struct Msg : vt::Message {
  NodeType broot;
  int bcastsLeft;

  Msg(
    NodeType const& in_broot,
    int in_bcastsLeft
  ) : Message(), broot(in_broot), bcastsLeft(in_bcastsLeft) { }
};

static void bcastTest(Msg* msg) {
  auto const node = theContext()->getNode();
  auto const& root = msg->broot;

#if BCAST_DEBUG
  fmt::print("{}: bcastTestHandler root={} bcastsleft={}\n", node, msg->broot, msg->bcastsLeft);
#endif

  vtAssert(
    root != my_node, "Broadcast should deliver to all but this node"
  );

#if BCAST_DEBUG
  fmt::print("{}: waiting on barrier before initiating sub bcast\n", node);
#endif

  // Wait on barrier as a result of RECIEVING a bcast
  theCollective()->barrier();

  if (msg->bcastsLeft > 1) {
    theMsg()->broadcastMsg<Msg, bcastTest>(
      makeSharedMessage<Msg>(node, msg->bcastsLeft - 1)
    );

    // Wait on barrier ourselves - as we don't get the message
    theCollective()->barrier();
  }

  bcast_count++;
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();

  if (theContext()->getNumNodes() == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  NodeType from_node = uninitialized_destination;

  if (argc > 1) {
    from_node = atoi(argv[1]);
  }

  int32_t const expected = num_bcasts *
    (from_node == uninitialized_destination ? theContext()->getNumNodes() - 1 : (
      from_node == my_node ? 0 : 1
    ));

  theTerm()->addAction([=]{
    fmt::print("[{}] verify: bcast_count={}, expected={}\n", my_node, bcast_count, expected);
    //    vtAssertExpr(bcast_count == expected);
  });

  if (from_node == uninitialized_destination or from_node == my_node) {
    fmt::print("[{}] broadcast_test: broadcasting {} times\n", my_node, num_bcasts);
    for (int i = 0; i < num_bcasts; i++) {
      theMsg()->broadcastMsg<Msg, bcastTest>(makeSharedMessage<Msg>(my_node, bcast_depth));
      // All receives will wait on the barrier - we should too.
      theCollective()->barrier();
    }
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
