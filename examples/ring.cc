
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static handler_t ring_han = uninitialized_handler;
static node_t next_node = uninitialized_destination;

static node_t my_node = uninitialized_destination;
static node_t num_nodes = uninitialized_destination;

static int num_total_rings = 2;
static int num_times = 0;

struct RingMsg : runtime::Message {
  int from;

  RingMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

static void send_to_next() {
  RingMsg* msg = new RingMsg(my_node);
  the_msg->send_msg(next_node, ring_han, msg, [=]{ delete msg; });
}

static void ring(runtime::BaseMessage* in_msg) {
  RingMsg& msg = *static_cast<RingMsg*>(in_msg);

  printf("%d: Hello from node %d: num_times=%d\n", my_node, msg.from, num_times);

  num_times++;

  if (msg.from != num_nodes-1 or num_times < num_total_rings) {
    send_to_next();
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  ring_han = the_msg->collective_register_handler(ring);

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  next_node = my_node+1 >= num_nodes ? 0 : my_node+1;

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    send_to_next();
  }

  while (1) {
    the_msg->scheduler();
  }

  return 0;
}
