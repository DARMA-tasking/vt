
#include "transport.h"

#include <iostream>

struct TestMsg : runtime::Message {
  int val;
  int val2;

  TestMsg(int const& in_val, int const& in_val2)
    : val(in_val), val2(in_val2)
  { }
};

void handle_test_msg(runtime::Message* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  std::cout << "val=" << msg.val << ", "
            << "val2=" << msg.val2
            << std::endl;
}

using namespace runtime;

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);

  node_t this_node = the_context->get_node();
  node_t num_nodes = the_context->get_num_nodes();

  std::cout << "this_node=" << this_node << std::endl;

  auto test_msg_han = CollectiveOps::register_handler(handle_test_msg);

  TestMsg* msg = new TestMsg(this_node, num_nodes);

  event_t evt = the_msg->send_msg(0, test_msg_han, msg, [=]{
    std::cout << "deleting msg" << std::endl;
    delete msg;
  });

  while (1) {
    the_msg->scheduler();
  }

  CollectiveOps::finalize_context();
}
