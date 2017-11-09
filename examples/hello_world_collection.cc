
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::vrt::collection;
using namespace vt::index;
using namespace vt::mapping;

struct MyCol : Collection<Index1D> {
  MyCol(VirtualElmCountType elms, Index1D idx) : Collection<Index1D>(elms) {
    auto const& node = theContext()->getNode();
    printf("constructing MyCol on node=%d: idx.x()=%d\n", node, idx.x());
  }
};


struct HelloMsg : vt::Message {
  int from;

  HelloMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

static void hello_world(HelloMsg* msg) {
  printf("%d: Hello from node %d\n", theContext()->getNode(), msg->from);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    auto proxy = theCollection()->makeCollection<
      MyCol, Index1D, defaultDenseIndex1DMap
    >(Index1D(64));
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
