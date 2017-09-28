
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::vrt;

struct HelloMsg : vt::Message {
  VrtContext_ProxyType proxy;

  HelloMsg(VrtContext_ProxyType const& in_proxy)
    : Message(), proxy(in_proxy)
  { }
};

struct TestMsg : vt::vrt::VrtContextMessage {
  int from = 0;

  TestMsg(int const& in_from)
    : VrtContextMessage(), from(in_from)
  { }
};

struct MyVC : vt::vrt::VrtContext {
  int my_data = 10;

  MyVC(int const& my_data_in) : my_data(my_data_in) { }
};

static void my_han(TestMsg* msg, MyVC* vc) {
  auto this_node = theContext->getNode();
  printf(
    "%d: vc=%p: msg->from=%d, vc->my_data=%d\n",
    this_node, vc, msg->from, vc->my_data
  );
}

static void sendMsgToProxy(VrtContext_ProxyType const& proxy) {
  auto this_node = theContext->getNode();
  printf("%d: sendMsgToProxy: proxy=%llu\n", this_node, proxy);

  auto m = new TestMsg(this_node + 32);
  theVrtCManager->sendMsg<MyVC, TestMsg, my_han>(proxy, m, [=]{ delete m; });
}

static void hello_world(HelloMsg* msg) {
  auto this_node = theContext->getNode();
  printf("%d: hello: proxy=%llu\n", this_node, msg->proxy);
  sendMsgToProxy(msg->proxy);
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
    auto proxy = theVrtCManager->constructVrtContext<MyVC>(29);
    sendMsgToProxy(proxy);

    // send out the proxy to all the nodes
    HelloMsg* msg = new HelloMsg(proxy);
    theMsg->broadcastMsg<HelloMsg, hello_world>(msg, [=]{ delete msg; });
  }

  while (vtIsWorking) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
