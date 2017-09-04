
#include "transport.h"
#include <cstdlib>

using namespace runtime;

struct TestMsg : runtime::Message {
  node_t from;
  handler_t callback_han;

  TestMsg(node_t const& in_from, handler_t const& in_callback_han)
    : Message(), from(in_from), callback_han(in_callback_han)
  { }
};

static void callback_fn(runtime::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  printf("%d: local handler node %d\n", the_context->get_node(), msg.from);
}

static void my_col_fn(TestMsg* msg) {
  auto const& my_node = the_context->get_node();

  printf(
    "%d: my_col_fn from=%d, callback=%d: sending\n",
    my_node, msg->from, msg->callback_han
  );

  TestMsg* new_msg = make_shared_message<TestMsg>(my_node, uninitialized_handler);
  the_msg->send_msg(msg->callback_han, new_msg);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  handler_t const callback = the_msg->register_new_handler(callback_fn);

  auto const& my_node = the_context->get_node();
  auto const& num_nodes = the_context->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    TestMsg* msg = new TestMsg(my_node, callback);
    the_msg->broadcast_msg<TestMsg, my_col_fn>(msg, [=]{ delete msg; });
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
