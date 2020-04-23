/*
//@HEADER
// *****************************************************************************
//
//                          termination_collective.cc
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

using TestMsg = vt::Message;

vt::NodeType nextNode() {
  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();
  return (this_node + 1) % num_nodes;
}

static void test_handler(TestMsg* msg) {
  static int num = 3;

  vt::NodeType this_node = vt::theContext()->getNode();

  auto epoch = vt::envelopeGetEpoch(msg->env);
  fmt::print("{}: test_handler: num={}, epoch={:x}\n", this_node, num, epoch);

  num--;
  if (num > 0) {
    auto msg_send = vt::makeMessage<TestMsg>();
    vt::theMsg()->sendMsg<TestMsg, test_handler>(nextNode(), msg_send.get());
  }
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  auto epoch = vt::theTerm()->makeEpochCollective();

  // This action will not run until all messages originating from the
  // sends are completed
  vt::theTerm()->addAction(epoch, [=]{
    fmt::print("{}: finished epoch={:x}\n", this_node, epoch);
  });

  // Message must go out of scope before finalize
  {
    auto msg = vt::makeMessage<TestMsg>();
    vt::envelopeSetEpoch(msg->env, epoch);
    vt::theMsg()->sendMsg<TestMsg, test_handler>(nextNode(), msg.get());
  }

  vt::theTerm()->finishedEpoch(epoch);

  vt::finalize();

  return 0;
}
