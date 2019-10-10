/*
//@HEADER
// *****************************************************************************
//
//                              group_example_1.cc
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

  HelloMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

struct HelloGroupMsg : ::vt::Message {
  HelloGroupMsg() = default;
};

static void hello_world(HelloMsg* msg) {
  fmt::print("{}: Hello from node {}\n", theContext()->getNode(), msg->from);
}

static void hello_group_handler(HelloGroupMsg* msg) {
  fmt::print("{}: Hello from group handler\n", theContext()->getNode());
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  if (my_node == 0) {
    HelloMsg* msg = makeSharedMessage<HelloMsg>(my_node);
    theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);

    //std::vector<region::Region::BoundType> vec{0,1,2,3,4,5,6,7};
    //auto list = std::make_unique<region::List>(vec);
    auto list = std::make_unique<region::Range>(
      theContext()->getNumNodes() / 2,
      theContext()->getNumNodes()
    );
    this_group = theGroup()->newGroup(std::move(list), [](GroupType group){
      fmt::print("Group is created\n");
      auto gmsg = makeSharedMessage<HelloGroupMsg>();
      envelopeSetGroup(gmsg->env, group);
      theMsg()->broadcastMsg<HelloGroupMsg, hello_group_handler>(gmsg);
    });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
