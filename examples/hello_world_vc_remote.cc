
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::vrt;

struct TestMsg : vt::vrt::VirtualMessage {
  int from = 0;

  TestMsg(int const& in_from)
    : VirtualMessage(), from(in_from)
  { }
};

struct MyVC : vt::vrt::VirtualContext {
  int my_data = -1;

  MyVC(int const& my_data_in) : my_data(my_data_in) {
    printf("constructing myVC: data=%d\n", my_data_in);
  }
};

static void testHan(TestMsg* msg, MyVC* vc) {
  printf("testHan: msg->from=%d, my_data=%d\n", msg->from, vc->my_data);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    auto proxy = theVirtualManager()->makeVirtualNode<MyVC>(1, 45);
    auto msg = new TestMsg(my_node);
    theVirtualManager()->sendMsg<MyVC, TestMsg, testHan>(proxy, msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
