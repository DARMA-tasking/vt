
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;

struct TestMsg : vt::Message {
  NodeType broot;

  TestMsg(NodeType const& in_broot) : Message(), broot(in_broot) { }
};

static void bcastTest(TestMsg* msg) {
  auto const& root = msg->broot;

  printf(
    "%d: bcastTestHandler root=%d\n", theContext->getNode(), msg->broot
  );

  assert(
    root != my_node and "Broadcast should deliver to all but this node"
  );
}

int main(int argc, char** argv) {
  CollectiveOps::initializeContext(argc, argv);
  CollectiveOps::initializeRuntime();

  my_node = theContext->getNode();

  if (theContext->getNumNodes() == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  NodeType from_node = uninitialized_destination;

  if (argc > 1) {
    from_node = atoi(argv[1]);
  }

  if (from_node == uninitialized_destination or from_node == my_node) {
    theMsg->broadcastMsg<TestMsg, bcastTest>(makeSharedMessage<TestMsg>(my_node));
  }

  while (1) {
    runScheduler();
  }

  return 0;
}
