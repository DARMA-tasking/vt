/*
//@HEADER
// *****************************************************************************
//
//                              group_example_1.cc
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

#include <vt/transport.h>

struct HelloMsg : vt::Message {
  vt::NodeType from = vt::uninitialized_destination;

  explicit HelloMsg(vt::NodeType const& in_from)
    : from(in_from)
  { }
};

static void hello_world(HelloMsg* msg) {
  fmt::print("{}: Hello from node {}\n", vt::theContext()->getNode(), msg->from);
}

static void hello_group_handler(HelloMsg* msg) {
  fmt::print("{}: Hello from group {}\n", vt::theContext()->getNode(), msg->from);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  if (this_node == 0) {
    auto msg = vt::makeMessage<HelloMsg>(this_node);
    vt::theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);

    using RangeType = vt::group::region::Range;
    auto list = std::make_unique<RangeType>(num_nodes / 2, num_nodes);

    vt::theGroup()->newGroup(std::move(list), [=](vt::GroupType group){
      auto gmsg = vt::makeMessage<HelloMsg>(this_node);
      vt::envelopeSetGroup(gmsg->env, group);
      vt::theMsg()->broadcastMsg<HelloMsg, hello_group_handler>(gmsg);
    });
  }

  vt::finalize();

  return 0;
}
