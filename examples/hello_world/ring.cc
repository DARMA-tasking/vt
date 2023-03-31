/*
//@HEADER
// *****************************************************************************
//
//                                   ring.cc
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

#include <vt/transport.h>

static int num_total_rings = 2;
static int num_times = 0;

struct RingMsg : vt::Message {
  vt::NodeType from = vt::uninitialized_destination;

  explicit RingMsg(vt::NodeType const& in_from)
    : from(in_from)
  { }
};

static void sendToNext();

static void ring(RingMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  fmt::print("{}: Hello from node {}: num_times={}\n", this_node, msg->from, num_times);

  num_times++;

  if (msg->from != num_nodes - 1 or num_times < num_total_rings) {
    sendToNext();
  }
}

static void sendToNext() {
  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();
  vt::NodeType next_node = this_node + 1 >= num_nodes ? 0 : this_node + 1;

  auto msg = vt::makeMessage<RingMsg>(this_node);
  vt::theMsg()->sendMsg<ring>(next_node, msg);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  if (this_node == 0) {
    sendToNext();
  }

  vt::finalize();

  return 0;
}
