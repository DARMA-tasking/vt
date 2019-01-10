/*
//@HEADER
// ************************************************************************
//
//                          trace_ring.cc
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

static NodeType next_node = uninitialized_destination;
static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static int64_t kernel_weight = 1000;

static int num_total_rings = 10;
static int num_times = 0;

struct RingMsg : vt::Message {
  NodeType from;
  RingMsg(NodeType const& in_from) : Message(), from(in_from) { }
};

static void sendToNext();

static double kernel(NodeType const& from_node) {
  double my_val = 19.234;
  for (int i = 0; i < kernel_weight; i++) {
    my_val += i - (2.34-1.28882) * from_node;
  }
  return my_val;
}

static void ring(RingMsg* msg) {
  auto const& ring_from_node = msg->from;

  num_times++;

  double const val = kernel(ring_from_node);

  fmt::print(
    "{}: Hello from node {}: num_times={}: kernel val={}\n",
    my_node, ring_from_node, num_times, val
  );

  if (ring_from_node != num_nodes-1 or num_times < num_total_rings) {
    sendToNext();
  }
}

static void sendToNext() {
  RingMsg* msg = new RingMsg(my_node);
  theMsg()->sendMsg<RingMsg, ring>(next_node, msg);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();
  next_node = my_node+1 >= num_nodes ? 0 : my_node+1;

  fmt::print("{}: my_node = {} here\n",theContext()->getNode(),my_node);

  if (num_nodes == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  if (argc > 1) {
    kernel_weight = atoi(argv[1]);
  }

  if (my_node == 0) {
    sendToNext();
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
