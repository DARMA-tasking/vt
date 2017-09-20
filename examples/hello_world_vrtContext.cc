
#include <cstdlib>
#include <iostream>

#include "transport.h"
#include "vrt/vrt_context.h"
#include "vrt/vrt_contextmanager.h"

using namespace vt;
using namespace vt::vrt;

struct HelloMsg : vt::Message {
  int from;

  explicit HelloMsg(int const& in_from)
      : Message(), from(in_from) {}
};

struct HelloVrtContext : VrtContext {
  int from;

  explicit HelloVrtContext(int const& in_from)
      : VrtContext(), from(in_from) {}
};

static void hello_world(HelloMsg *msg) {
  printf("%d: Hello from node %d\n", theContext->getNode(), msg->from);
}

int main(int argc, char **argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext->getNode();
  auto const& num_nodes = theContext->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  std::unique_ptr<vrt::VrtContextManager> theVrtCManager_ =
      std::make_unique<vrt::VrtContextManager>();

  if (my_node == 0) {

    auto vrtc = theVrtCManager_->newVrtContext();
    auto vrtc1 = theVrtCManager_->newVrtContext();

    vrtc.printVrtContext();
    vrtc1.printVrtContext();

    HelloVrtContext my_vrtc(20);

    my_vrtc.printVrtContext();

    auto myHelloVrtC_proxy =
        theVrtCManager_->newVrtContext<HelloVrtContext>(&my_vrtc);

    my_vrtc.printVrtContext();
    std::cout << my_vrtc.from << std::endl;


//    my_vrtc.printVrtContext();
//    theVrtCManager->newVrtContext(my_vrtc);
//    my_vrtc.printVrtContext();
//    vrtc.printVrtContext();
//    vrtc1.printVrtContext();
////    std::cout << "My node: " << vrtc.getVrtContextNode() << std::endl;
////    std::cout << theVrtCManager_->newVrtContext() << std::endl;
//
//    std::cout << my_vrtc.getVrtContextUId() << std::endl;
//    std::cout << my_vrtc.isCollection() << std::endl;

    HelloMsg *msg = new HelloMsg(my_node);
    theMsg->broadcastMsg<HelloMsg, hello_world>(msg, [=] { delete msg; });
  }

  while (vtIsWorking) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
