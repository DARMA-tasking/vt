
#include "transport.h"
#include <cstdlib>

using namespace vt;

static HandlerType my_col_han = uninitialized_handler;
static HandlerType my_reinstate_fn = uninitialized_handler;

struct TestMsg : vt::Message {
  NodeType from;
  HandlerType callback_han;

  TestMsg(NodeType const& in_from, HandlerType const& in_callback_han)
    : Message(), from(in_from), callback_han(in_callback_han)
  { }
};

static void callbackFn2(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  printf("%d: callbackFn2 handler node %d\n", theContext()->getNode(), msg.from);
}

static void reinstateFn(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  // register a new function for the handler to deliver the rest of the msgs
  theMsg()->registerHandlerFn(msg.callback_han, callbackFn2);
}

static void callbackFn(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  HandlerType const& han = theMsg()->getCurrentHandler();
  theMsg()->unregisterHandlerFn(han);

  theMsg()->sendMsg(my_reinstate_fn, makeSharedMessage<TestMsg>(0, han));

  printf("%d: callbackFn handler node %d\n", theContext()->getNode(), msg.from);
}

static void myColFn(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  auto const& my_node = theContext()->getNode();

  printf(
    "%d: myColFn from=%d, callback=%d: sending\n",
    my_node, msg.from, msg.callback_han
  );

  TestMsg* new_msg = makeSharedMessage<TestMsg>(my_node, uninitialized_handler);
  theMsg()->sendMsg(msg.callback_han, new_msg);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  HandlerType const callback = theMsg()->registerNewHandler(callbackFn);
  my_reinstate_fn = theMsg()->registerNewHandler(reinstateFn);

  my_col_han = theMsg()->collectiveRegisterHandler(myColFn);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
    

  }

  if (my_node == 0) {
    TestMsg* msg = new TestMsg(my_node, callback);
    theMsg()->broadcastMsg(my_col_han, msg, [=]{ delete msg; });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
