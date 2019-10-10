/*
//@HEADER
// *****************************************************************************
//
//                                  vt_mpi.cc
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

#include <iostream>
#include <cstdio>

using namespace vt;

NodeType this_node = -1;
NodeType num_nodes = -1;
HandlerType test_msg_han = 0;
HandlerType test_msg_han2 = 0;
EpochType test_epoch = -1;

struct TestMsg : vt::Message {
  int val;
  int val2;
  EventType event;

  TestMsg(int const& in_val, int const& in_val2, EventType const& in_event)
    : val(in_val), val2(in_val2), event(in_event)
  { }
};

void handle_test_msg(vt::Message* in_msg) {
}

void send_to_neighbor() {
  auto msg = makeSharedMessage<TestMsg>(this_node, num_nodes, -1);

  int const next = this_node+1 < num_nodes ? this_node+1 : 0;

  //theMsg()->set_epoch_message(msg, test_epoch);

  EventType evt = theMsg()->sendMsg(next, test_msg_han, msg, [=]{
    //std::cout << "deleting msg" << std::endl;
    delete msg;
  });

  auto msg2 = makeSharedMessage<TestMsg>(this_node, num_nodes, evt);

  EventType evt2 = theMsg()->sendMsg(next, test_msg_han2, msg2, [=]{
    //std::cout << "deleting msg" << std::endl;
    delete msg2;
  });

  auto const& own_node = theEvent()->get_owning_node(evt);

  fmt::print(
    "this_node={}, event_id={}, own_node={}\n",
    this_node, evt, own_node
  );

  // std::cout << "this_node=" << this_node << ", "
  //           << "event_id=" << evt << ", "
  //           << "own_node=" << own_node
  //           << std::endl;
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  this_node = theContext()->getNode();
  num_nodes = theContext()->get_num_nodes();

  std::cout << "this_node=" << this_node << std::endl;

  fmt::print("sizeof(Envelope)={}\n", sizeof(Envelope));
  fmt::print("sizeof(EpochEnvelope)={}\n", sizeof(EpochEnvelope));
  fmt::print("sizeof(EpochTagEnvelope)={}\n", sizeof(EpochTagEnvelope));

  fmt::print("{}: calling wait_unnamed_barrier\n", this_node);
  theCollective()->barrier();
  fmt::print("{}: out of wait_unnamed_barrier\n", this_node);

  fmt::print("{}: calling cont_unnamed_barrier\n", this_node);
  theCollective()->barrier_then([=]{
    fmt::print("{}: out of cont_unnamed_barrier\n", this_node);
  });

  //test_msg_han = theMsg()->collective_register_handler(handle_test_msg);

  // test_epoch = theTerm()->new_epoch();
  // theTerm()->attach_epoch_term_action(test_epoch, [=]{
  //   fmt::print(
  //     "{}: EPOCH: finished: test_epoch={}\n",
  //     theContext()->getNode(), test_epoch
  //   );
  // });

  test_msg_han = theMsg()->collective_register_handler([](vt::BaseMessage* in_msg){
    TestMsg& msg = *static_cast<TestMsg*>(in_msg);

    fmt::print(
      "this_node={}, from_node={}, num_nodes={}\n",
      this_node, msg.val, msg.val2
    );

    if (this_node != 0) {
      send_to_neighbor();
    }
  });

  test_msg_han2 = theMsg()->collective_register_handler([](vt::BaseMessage* in_msg){
    TestMsg& msg = *static_cast<TestMsg*>(in_msg);

    fmt::print(
      "this_node={}, evt={}, owner={}\n",
      this_node, msg.event, theEvent()->get_owning_node(msg.event)
    );

    theEvent()->attach_action(msg.event, [=]{
      fmt::print("triggering remote event\n");
    });
  });

  if (this_node == 0) {
    send_to_neighbor();

    auto msg = makeSharedMessage<TestMsg>(this_node, num_nodes, -1);

    // theMsg()->broadcast_msg(test_msg_han, msg, [=]{
    //   //std::cout << "deleting msg" << std::endl;
    //   delete msg;
    // });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
