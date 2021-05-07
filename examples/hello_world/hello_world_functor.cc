/*
//@HEADER
// *****************************************************************************
//
//                            hello_world_functor.cc
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

#include <vt/transport.h>

struct HelloMsg : vt::Message {
  int from;

  explicit HelloMsg(int const& in_from)
    : from(in_from)
  { }
};

struct AnotherMsg : vt::Message{};

struct HelloWorld {
  void operator()(HelloMsg* msg) const {
    fmt::print("{}: HelloWorld -> Hello from node {}\n", vt::theContext()->getNode(), msg->from);
  }
};

struct MultipleFunctions {
  void operator()(HelloMsg* msg) const {
    fmt::print("{}: MultipleFunctions -> Hello from node {}\n", vt::theContext()->getNode(), msg->from);
  }

  void operator()(AnotherMsg* msg) const {
    fmt::print("{}: MultipleFunctions with AnotherMsg\n", vt::theContext()->getNode());
  }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  if (this_node == 0) {
    auto msg = vt::makeMessage<HelloMsg>(this_node);

    // 'HelloWorld' functor has only single 'operator()' declared
    // so we can call send/broadcast without specifying message type
    vt::theMsg()->broadcastMsg<HelloWorld>(msg);

    msg = vt::makeMessage<HelloMsg>(this_node);
    vt::theMsg()->sendMsg<HelloWorld>(1, msg);

    // 'MultipleFunctions' functor declares more than one 'operator()'
    // so we have to specify the type of the message, as it can't be deduced
    auto new_msg = vt::makeMessage<AnotherMsg>();
    vt::theMsg()->broadcastMsg<MultipleFunctions, AnotherMsg>(new_msg);

    new_msg = vt::makeMessage<AnotherMsg>();
    vt::theMsg()->sendMsg<MultipleFunctions, AnotherMsg>(1, new_msg);
  }

  vt::finalize();

  return 0;
}
