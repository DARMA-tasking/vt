
#include "vt/transport.h"
#include <cstdlib>

using namespace vt;

struct TestMsg : vt::Message {
  NodeType from;
  HandlerType callback_han;

  TestMsg(NodeType const& in_from, HandlerType const& in_callback_han)
    : Message(), from(in_from), callback_han(in_callback_han)
  { }
};

static void callback_fn(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  fmt::print("{}: local handler node {}\n", theContext()->getNode(), msg.from);
}

static void my_col_fn(TestMsg* msg) {
  auto const& my_node = theContext()->getNode();

  fmt::print(
    "{}: my_col_fn from={}, callback={}: sending\n",
    my_node, msg->from, msg->callback_han
  );

  TestMsg* new_msg = makeSharedMessage<TestMsg>(my_node, uninitialized_handler);
  theMsg()->sendMsg(msg->callback_han, new_msg);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  HandlerType const callback = theMsg()->registerNewHandler(callback_fn);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    TestMsg* msg = new TestMsg(my_node, callback);
    theMsg()->broadcastMsg<TestMsg, my_col_fn>(msg, [=]{ delete msg; });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
