
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

struct TestMsg : CallbackMessage {
  NodeType from;

  TestMsg(NodeType const& in_from) : CallbackMessage(), from(in_from) { }
};

static void test_msg_recv(TestMsg* msg) {
  printf("%d: sending callback %d\n", the_context->get_node(), msg->from);

  TestMsg* send_msg = make_shared_message<TestMsg>(my_node);
  the_msg->send_callback(send_msg);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    for (int cur_node = 0; cur_node < num_nodes; cur_node++) {
      TestMsg* msg = make_shared_message<TestMsg>(my_node);
      the_msg->send_msg_callback<TestMsg, test_msg_recv>(
        cur_node, msg, [=](BaseMessage* in_msg){
          TestMsg* msg = static_cast<TestMsg*>(in_msg);
          printf("%d: callback received from %d\n", the_context->get_node(), msg->from);
        }
      );
    }
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
