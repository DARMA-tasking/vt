
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
    printf("%d: Hello from node %d\n", theContext->getNode(), msg->from);
  }
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext->getNode();
  auto const& num_nodes = theContext->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    HelloMsg* msg = makeSharedMessage<HelloMsg>(my_node);
    theMsg->broadcastMsg<HelloWorld>(msg);
  }

  while (vtIsWorking) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
