/*
//@HEADER
// *****************************************************************************
//
//                             group_collective.cc
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

/// [Collective group creation]
struct HelloGroupMsg : vt::Message { };

static void hello_group_handler(HelloGroupMsg* msg) {
  fmt::print("{}: Hello from group handler\n", vt::theContext()->getNode());
}

using ReduceMsg = vt::collective::ReduceTMsg<int>;

struct Print {
  void operator()(ReduceMsg* msg) {
    fmt::print("final value={}\n", msg->getConstVal());
  }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes < 2) {
    return vt::rerror("requires at least 2 nodes");
  }

  srand48(this_node * 29);

  bool odd_node_filter = this_node % 2 == 1;

  vt::GroupType new_group = vt::theGroup()->newGroupCollective(
    odd_node_filter, [=](vt::GroupType group){
      auto const& root = 0;
      auto const& in_group = vt::theGroup()->inGroup(group);
      auto const& root_node = vt::theGroup()->groupRoot(group);
      auto const& is_default_group = vt::theGroup()->isGroupDefault(group);
      fmt::print(
        "{}: Group is created: group={:x}, in_group={}, root={}, "
        "is_default_group={}\n",
        this_node, group, in_group, root_node, is_default_group
      );
      if (in_group) {
        using Op = vt::collective::PlusOp<int>;
        auto msg = vt::makeMessage<ReduceMsg>(1);
        vt::theGroup()->groupReducer(group)->reduce<Op, Print>(root, msg.get());
      }
      if (this_node == 1) {
        auto msg = vt::makeMessage<HelloGroupMsg>();
        vt::envelopeSetGroup(msg->env, group);
        vt::theMsg()->broadcastMsg<HelloGroupMsg, hello_group_handler>(msg);
      }
    }
  );

  fmt::print("{}: New group={}\n", this_node, new_group);

  vt::finalize();

  return 0;
}
/// [Collective group creation]
