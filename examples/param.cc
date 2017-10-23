
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static void fnTest(int a, int b, bool x) {
  printf("fn: a=%d, b=%d, x=%s\n", a, b, x ? "true" : "false");
}

static void fnTest2(int x, int y) {
  printf("fn2: x=%d,y=%d\n",x,y);
}

static void fnTest3(int x, double y) {
  printf("fn3: x=%d,y=%f\n",x,y);
}

struct FunctorTest1 {
  void operator()(int x, double y) const {
    printf("FunctorTest1: x=%d,y=%f\n",x,y);
  }
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    theParam()->sendData(1, buildData(10, 20, false), PARAM_FUNCTION_RHS(fnTest));
    theParam()->sendData(1, PARAM_FUNCTION_RHS(fnTest), 50, 29, false);
    theParam()->sendData<PARAM_FUNCTION(fnTest)>(1, buildData(10, 20, false));
    theParam()->sendData<PARAM_FUNCTION(fnTest)>(1, 45, 23, true);

    theParam()->sendData<PARAM_FUNCTION(fnTest2)>(1, 20, 10);
    theParam()->sendData<PARAM_FUNCTION(fnTest3)>(1, 20, 50.0);

    theParam()->sendData<FunctorTest1>(1, buildData(20, 50.0));
    theParam()->sendData<FunctorTest1>(1, 20, 100.0);
    theParam()->sendData<FunctorTest1>(1, buildData(10, 70.0));
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
