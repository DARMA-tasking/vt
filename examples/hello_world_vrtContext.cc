
#include <cstdlib>
#include <iostream>

#include "transport.h"
#include "vrt/vrt_context.h"
#include "vrt/vrt_contextmanager.h"

using namespace vt;
using namespace vt::vrt;

struct HelloMsg : vt::Message {
  int from;

  HelloMsg(int const& in_from)
      : Message(), from(in_from)
  { }
};

static void hello_world(HelloMsg* msg) {
  printf("%d: Hello from node %d\n", theContext->getNode(), msg->from);
}

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

    VrtContext my_vrtc(0, 2);
    my_vrtc.setIsCollection(false);

    std::unique_ptr<vrt::VrtContextManager> theVrtCManager_ =
        std::make_unique<vrt::VrtContextManager>();

    theVrtCManager_->newVrtContext();
    std::cout << theVrtCManager_->newVrtContext() << std::endl;

    std::cout << my_vrtc.getVrtContextUId() << std::endl;
    std::cout << my_vrtc.isCollection() << std::endl;

    HelloMsg* msg = new HelloMsg(my_node);
    theMsg->broadcastMsg<HelloMsg, hello_world>(msg, [=]{ delete msg; });
  }

  while (vtIsWorking) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
