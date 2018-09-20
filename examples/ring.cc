
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType next_node = uninitialized_destination;
static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static int num_total_rings = 2;
static int num_times = 0;

struct RingMsg : vt::Message {
  int from;

  RingMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

static void sendToNext();

static void ring(RingMsg* msg) {
  fmt::print("{}: Hello from node {}: num_times={}\n", my_node, msg->from, num_times);

  num_times++;

  if (msg->from != num_nodes-1 or num_times < num_total_rings) {
    sendToNext();
  }
}

static void sendToNext() {
  RingMsg* msg = makeSharedMessage<RingMsg>(my_node);
  theMsg()->sendMsg<RingMsg, ring>(next_node, msg);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();
  next_node = my_node+1 >= num_nodes ? 0 : my_node+1;

  fmt::print("{}: my_node = {} here\n",theContext()->getNode(),my_node);

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
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
