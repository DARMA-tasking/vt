
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType next_node = uninitialized_destination;
static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static constexpr int64_t const kernel_weight = 10000000;

static int num_total_rings = 10;
static int num_times = 0;

struct RingMsg : vt::Message {
  NodeType from;
  RingMsg(NodeType const& in_from) : Message(), from(in_from) { }
};

static void send_to_next();

static double kernel(NodeType const& from_node) {
  double my_val = 19.234;
  for (int i = 0; i < kernel_weight; i++) {
    my_val += i - (2.34-1.28882) * from_node;
  }
  return my_val;
}

static void ring(RingMsg* msg) {
  auto const& ring_from_node = msg->from;

  num_times++;

  double const val = kernel(ring_from_node);

  printf(
    "%d: Hello from node %d: num_times=%d: kernel val=%f\n",
    my_node, ring_from_node, num_times, val
  );

  if (ring_from_node != num_nodes-1 or num_times < num_total_rings) {
    send_to_next();
  }
}

static void send_to_next() {
  RingMsg* msg = new RingMsg(my_node);
  theMsg->sendMsg<RingMsg, ring>(next_node, msg, [=]{ delete msg; });
}

int main(int argc, char** argv) {
  CollectiveOps::initializeContext(argc, argv);
  CollectiveOps::initializeRuntime();

  my_node = theContext->getNode();
  num_nodes = theContext->getNumNodes();
  next_node = my_node+1 >= num_nodes ? 0 : my_node+1;

  printf("%d: my_node = %d here\n",theContext->getNode(),my_node);

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    send_to_next();
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
