
#include "transport.h"
#include <cstdlib>

using namespace vt;

struct HelloMsg : vt::Message {
  int from;

  HelloMsg(int const& in_from) : from(in_from) { }
};

static void hello_world(HelloMsg* msg) {
  fmt::print("{}: Hello from node {}\n", theContext()->getNode(), msg->from);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    auto msg = makeSharedMessage<HelloMsg>(my_node);
    theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
