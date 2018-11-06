
#include "vt/transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static void fnTest(int a, int b, bool x) {
  fmt::print("fn: a={}, b={}, x={}\n", a, b, x ? "true" : "false");
}

static void fnTest2(int x, int y) {
  fmt::print("fn2: x={},y={}\n",x,y);
}

static void fnTest3(int x, double y) {
  fmt::print("fn3: x={},y={}\n",x,y);
}

struct FunctorTest1 {
  void operator()(int x, double y) const {
    fmt::print("FunctorTest1: x={},y={}\n",x,y);
  }
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  if (my_node == 0) {
    #if 0
    theParam()->sendData(1, buildData(10, 20, false), PARAM_FUNCTION_RHS(fnTest));
    theParam()->sendData(1, PARAM_FUNCTION_RHS(fnTest), 50, 29, false);
    theParam()->sendData<PARAM_FUNCTION(fnTest)>(1, buildData(10, 20, false));
    theParam()->sendData<PARAM_FUNCTION(fnTest)>(1, 45, 23, true);

    theParam()->sendData<PARAM_FUNCTION(fnTest2)>(1, 20, 10);
    theParam()->sendData<PARAM_FUNCTION(fnTest3)>(1, 20, 50.0);

    theParam()->sendData<FunctorTest1>(1, buildData(20, 50.0));
    theParam()->sendData<FunctorTest1>(1, 20, 100.0);
    theParam()->sendData<FunctorTest1>(1, buildData(10, 70.0));
    #endif
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
