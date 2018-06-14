
#include "scf.h"

#include <fmt/format.h>

#include <cstdlib>

using namespace ::vt;

static NodeType this_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

struct TestMsg : ::vt::Message {};

static void testHandler(TestMsg* msg) {
  ::fmt::print("{}: testHandler\n",this_node);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  this_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  ::fmt::print("{}: started\n",this_node);

  if (this_node == 0) {
    auto msg = makeSharedMessage<TestMsg>();
    theMsg()->broadcastMsg<TestMsg,testHandler>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
