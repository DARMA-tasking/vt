
#include "vt/transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::vrt;

struct HelloMsg : vt::Message {
  VirtualProxyType proxy;

  HelloMsg(VirtualProxyType const& in_proxy)
    : Message(), proxy(in_proxy)
  { }
};

struct TestMsg : vt::vrt::VirtualMessage {
  int from = 0;

  TestMsg(int const& in_from)
    : VirtualMessage(), from(in_from)
  { }
};

struct MyVC : vt::vrt::VirtualContext {
  int my_data = 10;

  MyVC(int const& my_data_in) : my_data(my_data_in) { }
};

static void my_han(TestMsg* msg, MyVC* vc) {
  auto this_node = theContext()->getNode();
  fmt::print(
    "{}: vc={}: msg->from={}, vc->my_data={}\n",
    this_node, print_ptr(vc), msg->from, vc->my_data
  );
}

static void sendMsgToProxy(VirtualProxyType const& proxy) {
  auto this_node = theContext()->getNode();
  fmt::print("{}: sendMsgToProxy: proxy={}\n", this_node, proxy);

  auto m = makeSharedMessage<TestMsg>(this_node + 32);
  theVirtualManager()->sendMsg<MyVC, TestMsg, my_han>(proxy, m);
}

static void hello_world(HelloMsg* msg) {
  auto this_node = theContext()->getNode();
  fmt::print("{}: hello: proxy={}\n", this_node, msg->proxy);
  sendMsgToProxy(msg->proxy);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    auto proxy = theVirtualManager()->makeVirtual<MyVC>(29);
    sendMsgToProxy(proxy);

    // send out the proxy to all the nodes
    HelloMsg* msg = makeSharedMessage<HelloMsg>(proxy);
    theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
