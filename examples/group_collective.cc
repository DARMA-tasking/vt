/*
//@HEADER
// *****************************************************************************
//
//                             group_collective.cc
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
using namespace vt::group;

static GroupType this_group = no_group;

struct HelloMsg : vt::Message {
  int from;

  explicit HelloMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

struct HelloGroupMsg : ::vt::Message {
  HelloGroupMsg() = default;
};

#pragma GCC diagnostic ignored "-Wunused-function"
static void hello_world(HelloMsg* msg) {
  fmt::print("{}: Hello from node {}\n", theContext()->getNode(), msg->from);
}

static void hello_group_handler(HelloGroupMsg* msg) {
  fmt::print("{}: Hello from group handler\n", theContext()->getNode());
}

struct SysMsg : collective::ReduceTMsg<int> {
  explicit SysMsg(int in_num)
    : ReduceTMsg<int>(in_num)
  { }
};

struct Print {
  void operator()(SysMsg* msg) {
    fmt::print("final value={}\n", msg->getConstVal());
  }
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  srand48(my_node * 29);

  //auto const& random_node_filter = drand48() < 0.5;
  //auto const& even_node_filter   = my_node % 2 == 0;
  auto const& odd_node_filter    = my_node % 2 == 1;

  this_group = theGroup()->newGroupCollective(
    odd_node_filter, [=](GroupType group){
      auto const& root = 0;
      auto const& in_group = theGroup()->inGroup(group);
      auto const& root_node = theGroup()->groupRoot(group);
      auto const& is_default_group = theGroup()->groupDefault(group);
      fmt::print(
        "{}: Group is created: group={}, in_group={}, root={}, "
        "is_default_group={}\n",
        my_node, group, in_group, root_node, is_default_group
      );
      if (in_group) {
        auto msg = makeSharedMessage<SysMsg>(1);
        //fmt::print("msg->num={}\n", msg->getConstVal());
        theGroup()->groupReduce(group)->reduce<
          SysMsg, SysMsg::msgHandler<SysMsg,collective::PlusOp<int>,Print>
        >(root, msg);
      }
      fmt::print("node={}\n", my_node);
      if (my_node == 1) {
        //vtAssertExpr(in_group);
        auto msg = makeSharedMessage<HelloGroupMsg>();
        envelopeSetGroup(msg->env, group);
        fmt::print("calling broadcasting={}\n", my_node);
        theMsg()->broadcastMsg<HelloGroupMsg, hello_group_handler>(msg);
      }
    }
  );

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
