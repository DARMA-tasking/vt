
#include "transport.h"
#include <cstdlib>

using namespace vt;

template <typename CallbackT>
struct HelloMsg : vt::Message {
  int from;
  CallbackT cb;

  HelloMsg(int const& in_from, CallbackT cb)
    : Message(), from(in_from), cb(cb)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | from;
    s | cb;
  }
};

struct TestMsg : vt::Message {};

template <typename CallbackT>
static void hello_world(HelloMsg<CallbackT>* msg) {
  fmt::print("{}: Hello from node {}\n", theContext()->getNode(), msg->from);
  auto nmsg = makeSharedMessage<TestMsg>();
  msg->cb.send(nmsg);
}

static void callback_method(TestMsg* msg) {
  fmt::print("{}: triggering callback_method\n", theContext()->getNode());
}
static void callback_method_2(TestMsg* msg) {
  fmt::print("{}: triggering callback_method_2\n", theContext()->getNode());
}

#define sstmac_app_name hello_world_vt
int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    auto cb = theCB()->makeCallbackSingleSendTyped<TestMsg,callback_method>(
      true, 2
    );
    // auto cb2 = theCB()->makeCallbackMultiSendTyped<
    //   TestMsg,callback_method,callback_method_2
    // >(true, 2);

    auto cb3a = theCB()->pushTarget<TestMsg,callback_method>(1);
    auto cb3b = theCB()->pushTarget<TestMsg,callback_method>(cb3a,2);
    auto cb3c = theCB()->pushTarget<TestMsg,callback_method_2>(cb3b,1);
    auto cb2 = theCB()->buildMultiCB(cb3c);

    auto msg = makeSharedMessage<HelloMsg<decltype(cb2)>>(my_node, cb2);
    theMsg()->sendMsg<HelloMsg<decltype(cb2)>, hello_world>(1, msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
