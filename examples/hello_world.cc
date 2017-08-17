
#include "transport.h"
#include <cstdlib>

using namespace runtime;

handler_t hello_world_han = uninitialized_handler;

struct HelloMsg : runtime::Message {
  int from;

  HelloMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

static void hello_world(runtime::BaseMessage* in_msg) {
  HelloMsg& msg = *static_cast<HelloMsg*>(in_msg);

  printf("%d: Hello from node %d\n", the_context->get_node(), msg.from);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  hello_world_han = CollectiveOps::register_handler(hello_world);

  auto const& my_node = the_context->get_node();
  auto const& num_nodes = the_context->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    HelloMsg* msg = new HelloMsg(my_node);
    the_msg->broadcast_msg(hello_world_han, msg, [=]{ delete msg; });

    // Example of  to use a system managed message with transfer of control
    // HelloMsg* msg = make_shared_message<HelloMsg>(my_node);
    // the_msg->broadcast_msg(hello_world_han, msg);
  }

  while (1) {
    the_msg->scheduler();
  }

  return 0;
}
