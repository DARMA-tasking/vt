/*
//@HEADER
// ************************************************************************
//
//                          tagged_handler.cc
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

static HandlerType my_reinstate_fn = uninitialized_handler;
static TagType const first_recv_tag = 10;
static TagType const last_recv_tag = 15;
static TagType cur_iter = first_recv_tag;
static int count = 0;

struct TestMsg : vt::Message {
  NodeType from;
  HandlerType callback_han;

  TestMsg(NodeType const& in_from, HandlerType const& in_callback_han)
    : Message(), from(in_from), callback_han(in_callback_han)
  { }
};

static void processIterMsgs(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  auto const& first_tag = envelopeGetTag(msg.env);

  count++;

  fmt::print(
    "{}: process iteration node {}: count={}, tag={}, iteration={}\n",
    theContext()->getNode(), msg.from, count, first_tag, cur_iter
  );

  vtAssertExpr(first_tag == cur_iter);

  // received all for this iteration
  if (count == theContext()->getNumNodes() - 1) {
    cur_iter++;
    count = 0;

    auto const& first_han = theMsg()->getCurrentHandler();
    theMsg()->unregisterHandlerFn(first_han, cur_iter-1);
    theMsg()->registerHandlerFn(first_han, processIterMsgs, cur_iter);

    fmt::print(
      "{}: updating to NEXT iteration node {}: count={}, cur_iter={}\n",
      theContext()->getNode(), msg.from, count, cur_iter
    );
  }
}

static void myColFn(TestMsg* msg) {
  auto const& my_node = theContext()->getNode();

  fmt::print(
    "{}: my_col_fn from={}, callback={}: tag={}, sending, tag=[{},{}]\n",
    my_node, msg->from, msg->callback_han, first_recv_tag, first_recv_tag, last_recv_tag
  );

  for (auto i = first_recv_tag; i < last_recv_tag; i++) {
    auto new_msg = makeSharedMessage<TestMsg>(my_node, uninitialized_handler);
    theMsg()->sendMsg<TestMsg>(msg->callback_han, new_msg, i);
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  HandlerType const callback = theMsg()->registerNewHandler(
    processIterMsgs, cur_iter
  );

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  if (my_node == 0) {
    TestMsg* msg = makeSharedMessage<TestMsg>(my_node, callback);
    theMsg()->broadcastMsg<TestMsg, myColFn>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
