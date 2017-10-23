
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType next_node = uninitialized_destination;
static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static int64_t kernel_weight = 1000;

static int num_total_rings = 10;
static int num_times = 0;

struct RingMsg : vt::Message {
  NodeType from;
  RingMsg(NodeType const& in_from) : Message(), from(in_from) { }
};

static void sendToNext();

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
    sendToNext();
  }
}

static void sendToNext() {
  RingMsg* msg = new RingMsg(my_node);
  theMsg()->sendMsg<RingMsg, ring>(next_node, msg, [=]{ delete msg; });
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();
  next_node = my_node+1 >= num_nodes ? 0 : my_node+1;

  printf("%d: my_node = %d here\n",theContext()->getNode(),my_node);

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (argc > 1) {
    kernel_weight = atoi(argv[1]);
  }

  if (my_node == 0) {
    sendToNext();
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
