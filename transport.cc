
#include "transport.h"

#include <iostream>
#include <cstdio>

using namespace runtime;

node_t this_node = -1;
node_t num_nodes = -1;
handler_t test_msg_han = 0;
handler_t test_msg_han2 = 0;

struct TestMsg : runtime::Message {
  int val;
  int val2;
  event_t event;

  TestMsg(int const& in_val, int const& in_val2, event_t const& in_event)
    : val(in_val), val2(in_val2), event(in_event)
  { }
};

void handle_test_msg(runtime::Message* in_msg) {
}

void send_to_neighbor() {
  TestMsg* msg = new TestMsg(this_node, num_nodes, -1);

  int const next = this_node+1 < num_nodes ? this_node+1 : 0;

  event_t evt = the_msg->send_msg(next, test_msg_han, msg, [=]{
    //std::cout << "deleting msg" << std::endl;
    delete msg;
  });

  TestMsg* msg2 = new TestMsg(this_node, num_nodes, evt);

  event_t evt2 = the_msg->send_msg(next, test_msg_han2, msg2, [=]{
    //std::cout << "deleting msg" << std::endl;
    delete msg2;
  });

  auto const& own_node = the_event->get_owning_node(evt);

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

  this_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  std::cout << "this_node=" << this_node << std::endl;

  //test_msg_han = CollectiveOps::register_handler(handle_test_msg);

  test_msg_han = CollectiveOps::register_handler([](runtime::Message* in_msg){
    TestMsg& msg = *static_cast<TestMsg*>(in_msg);

    printf(
      "this_node=%d, from_node=%d, num_nodes=%d\n",
      this_node, msg.val, msg.val2
    );

    // if (this_node != 0) {
    //   send_to_neighbor();
    // }
  });

  test_msg_han2 = CollectiveOps::register_handler([](runtime::Message* in_msg){
    TestMsg& msg = *static_cast<TestMsg*>(in_msg);

    printf(
      "this_node=%d, evt=%lld, owner=%d\n",
      this_node, msg.event, the_event->get_owning_node(msg.event)
    );

    the_event->attach_action(msg.event, [=]{
      printf("triggering remote event\n");
    });
  });

  if (this_node == 0) {
    //send_to_neighbor();

    TestMsg* msg = new TestMsg(this_node, num_nodes, -1);

    the_msg->broadcast_msg(test_msg_han, msg, [=]{
      //std::cout << "deleting msg" << std::endl;
      delete msg;
    });
  }

  while (1) {
    the_msg->scheduler();
  }

  CollectiveOps::finalize_runtime();
  CollectiveOps::finalize_context();
}
