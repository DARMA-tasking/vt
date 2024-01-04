#include "transport.h"

#include <iostream>
#include <cstdio>

using namespace vt;

NodeT this_node = -1;
NodeT num_nodes = -1;
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
  TestMsg* msg = new TestMsg(this_node, num_nodes, -1);

  int const next = this_node+1 < num_nodes ? this_node+1 : 0;

  //theMsg()->set_epoch_message(msg, test_epoch);

  EventType evt = theMsg()->sendMsg(next, test_msg_han, msg, [=]{
    //std::cout << "deleting msg" << std::endl;
    delete msg;
  });

  TestMsg* msg2 = new TestMsg(this_node, num_nodes, evt);

  EventType evt2 = theMsg()->sendMsg(next, test_msg_han2, msg2, [=]{
    //std::cout << "deleting msg" << std::endl;
    delete msg2;
  });

  auto const& own_node = vtheEvent->get_owning_node(evt);

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

  fmt::print("sizeof(Envelope)=%ld\n", sizeof(Envelope));
  fmt::print("sizeof(EpochEnvelope)=%ld\n", sizeof(EpochEnvelope));
  fmt::print("sizeof(EpochTagEnvelope)=%ld\n", sizeof(EpochTagEnvelope));

  fmt::print("{}: calling wait_unnamed_barrier\n", this_node);
  theCollective()->barrier();
  fmt::print("{}: out of wait_unnamed_barrier\n", this_node);

  fmt::print("{}: calling cont_unnamed_barrier\n", this_node);
  theCollective()->barrier_then([=]{
    fmt::print("{}: out of cont_unnamed_barrier\n", this_node);
  });

  //test_msg_han = CollectiveOps::register_handler(handle_test_msg);

  // test_epoch = theTerm()->new_epoch();
  // theTerm()->attach_epoch_term_action(test_epoch, [=]{
  //   fmt::print(
  //     "{}: EPOCH: finished: test_epoch={}\n",
  //     theContext()->getNode(), test_epoch
  //   );
  // });

  test_msg_han = CollectiveOps::register_handler([](vt::BaseMessage* in_msg){
    TestMsg& msg = *static_cast<TestMsg*>(in_msg);

    fmt::print(
      "this_node={}, from_node={}, num_nodes={}\n",
      this_node, msg.val, msg.val2
    );

    if (this_node != 0) {
      send_to_neighbor();
    }
  });

  test_msg_han2 = CollectiveOps::register_handler([](vt::BaseMessage* in_msg){
    TestMsg& msg = *static_cast<TestMsg*>(in_msg);

    fmt::print(
      "this_node={}, evt={}, owner={}\n",
      this_node, msg.event, vttheEvent->get_owning_node(msg.event)
    );

    vt-theEvent->attach_action(msg.event, [=]{
      fmt::print("triggering remote event\n");
    });
  });

  if (this_node == 0) {
    send_to_neighbor();

    TestMsg* msg = new TestMsg(this_node, num_nodes, -1);

    // theMsg()->broadcast_msg(test_msg_han, msg, [=]{
    //   //std::cout << "deleting msg" << std::endl;
    //   delete msg;
    // });
  }

  while (1) {
    theMsg()->scheduler();
  }
}
