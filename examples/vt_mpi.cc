
#include "transport.h"

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
  TestMsg* msg = new TestMsg(this_node, num_nodes, -1);

  int const next = this_node+1 < num_nodes ? this_node+1 : 0;

  //theMsg->set_epoch_message(msg, test_epoch);

  EventType evt = theMsg->sendMsg(next, test_msg_han, msg, [=]{
    //std::cout << "deleting msg" << std::endl;
    delete msg;
  });

  TestMsg* msg2 = new TestMsg(this_node, num_nodes, evt);

  EventType evt2 = theMsg->sendMsg(next, test_msg_han2, msg2, [=]{
    //std::cout << "deleting msg" << std::endl;
    delete msg2;
  });

  auto const& own_node = theEvent->get_owning_node(evt);

  printf(
    "this_node=%d, event_id=%lld, own_node=%d\n",
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

  this_node = theContext->getNode();
  num_nodes = theContext->get_num_nodes();

  std::cout << "this_node=" << this_node << std::endl;

  printf("sizeof(Envelope)=%ld\n", sizeof(Envelope));
  printf("sizeof(EpochEnvelope)=%ld\n", sizeof(EpochEnvelope));
  printf("sizeof(EpochTagEnvelope)=%ld\n", sizeof(EpochTagEnvelope));

  printf("%d: calling wait_unnamed_barrier\n", this_node);
  theBarrier->barrier();
  printf("%d: out of wait_unnamed_barrier\n", this_node);

  printf("%d: calling cont_unnamed_barrier\n", this_node);
  theBarrier->barrier_then([=]{
    printf("%d: out of cont_unnamed_barrier\n", this_node);
  });

  //test_msg_han = theMsg->collective_register_handler(handle_test_msg);

  // test_epoch = theTerm->new_epoch();
  // theTerm->attach_epoch_term_action(test_epoch, [=]{
  //   printf(
  //     "%d: EPOCH: finished: test_epoch=%d\n",
  //     theContext->getNode(), test_epoch
  //   );
  // });

  test_msg_han = theMsg->collective_register_handler([](vt::BaseMessage* in_msg){
    TestMsg& msg = *static_cast<TestMsg*>(in_msg);

    printf(
      "this_node=%d, from_node=%d, num_nodes=%d\n",
      this_node, msg.val, msg.val2
    );

    if (this_node != 0) {
      send_to_neighbor();
    }
  });

  test_msg_han2 = theMsg->collective_register_handler([](vt::BaseMessage* in_msg){
    TestMsg& msg = *static_cast<TestMsg*>(in_msg);

    printf(
      "this_node=%d, evt=%lld, owner=%d\n",
      this_node, msg.event, theEvent->get_owning_node(msg.event)
    );

    theEvent->attach_action(msg.event, [=]{
      printf("triggering remote event\n");
    });
  });

  if (this_node == 0) {
    send_to_neighbor();

    TestMsg* msg = new TestMsg(this_node, num_nodes, -1);

    // theMsg->broadcast_msg(test_msg_han, msg, [=]{
    //   //std::cout << "deleting msg" << std::endl;
    //   delete msg;
    // });
  }

  while (1) {
    run_scheduler();
  }
}
