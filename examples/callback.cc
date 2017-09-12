
#include "transport.h"
#include <cstdlib>

using namespace vt;

struct TestMsg : vt::Message {
  NodeType from;
  HandlerType callback_han;

  TestMsg(NodeType const& in_from, HandlerType const& in_callback_han)
    : Message(), from(in_from), callback_han(in_callback_han)
  { }
};

static void callback_fn(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  printf("%d: local handler node %d\n", theContext->get_node(), msg.from);
}

static void my_col_fn(TestMsg* msg) {
  auto const& my_node = theContext->get_node();

  printf(
    "%d: my_col_fn from=%d, callback=%d: sending\n",
    my_node, msg->from, msg->callback_han
  );

  TestMsg* new_msg = make_shared_message<TestMsg>(my_node, uninitialized_handler);
  theMsg->send_msg(msg->callback_han, new_msg);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  HandlerType const callback = theMsg->register_new_handler(callback_fn);

  auto const& my_node = theContext->get_node();
  auto const& num_nodes = theContext->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    TestMsg* msg = new TestMsg(my_node, callback);
    theMsg->broadcast_msg<TestMsg, my_col_fn>(msg, [=]{ delete msg; });
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
