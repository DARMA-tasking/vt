
#include "transport.h"
#include <cstdlib>

using namespace vt;

struct HelloMsg : vt::Message {
  int from;

  HelloMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

struct HelloWorld {
  void operator()(HelloMsg* msg) const {
    printf("%d: Hello from node %d\n", the_context->get_node(), msg->from);
  }
};

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  auto const& my_node = the_context->get_node();
  auto const& num_nodes = the_context->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    HelloMsg* msg = make_shared_message<HelloMsg>(my_node);
    the_msg->broadcast_msg<HelloWorld>(msg);
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
