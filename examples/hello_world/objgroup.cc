/*
//@HEADER
// *****************************************************************************
//
//                                 objgroup.cc
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

/// [Object group creation]
struct MyObjGroup {
  void handler(int a, int b) {
    auto node = vt::theContext()->getNode();
    fmt::print("{}: MyObjGroup::handler on a={}, b={}\n", node, a, b);
  }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  const auto this_node = vt::theContext()->getNode();
  const auto num_nodes = vt::theContext()->getNumNodes();

  auto proxy =
    vt::theObjGroup()->makeCollective<MyObjGroup>("examples_hello_world");

  // Create group of odd nodes and broadcast to them (from root node)
  vt::theGroup()->newGroupCollective(
    this_node % 2, [proxy, this_node](::vt::GroupType type) {
      if (this_node == 0) {
        proxy.multicast<&MyObjGroup::handler>(type, 122, 244);
      }
    });

  vt::theCollective()->barrier();

  if (this_node == 0) {
    // Send to object 0
    proxy[0].send<&MyObjGroup::handler>(5, 10);
    if (num_nodes > 1) {
      // Send to object 1
      proxy[1].send<&MyObjGroup::handler>(10, 20);
    }

    // Broadcast to all nodes
    proxy.broadcast<&MyObjGroup::handler>(400, 500);

    using namespace ::vt::group::region;

    // Create list of nodes and broadcast to them
    List::ListType range;
    for (vt::NodeType node = 0; node < num_nodes; ++node) {
      if (node % 2 == 0) {
        range.push_back(node);
      }
    }

    proxy.multicast<&MyObjGroup::handler>(
      std::make_unique<List>(range), 20, 40
    );
  }
  vt::theCollective()->barrier();

  vt::finalize();

  return 0;
}
/// [Object group creation]
