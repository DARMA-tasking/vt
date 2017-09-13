
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

static void callback_fn_2(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  printf("%d: callback_fn_2 handler node %d\n", theContext->getNode(), msg.from);
}

static void reinstate_fn(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  // register a new function for the handler to deliver the rest of the msgs
  theMsg->registerHandlerFn(msg.callback_han, callback_fn_2);
}

static void callback_fn(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  HandlerType const& han = theMsg->getCurrentHandler();
  theMsg->unregisterHandlerFn(han);

  theMsg->sendMsg(my_reinstate_fn, make_shared_message<TestMsg>(0, han));

  printf("%d: callback_fn handler node %d\n", theContext->getNode(), msg.from);
}

static void my_col_fn(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  auto const& my_node = theContext->getNode();

  printf(
    "%d: my_col_fn from=%d, callback=%d: sending\n",
    my_node, msg.from, msg.callback_han
  );

  TestMsg* new_msg = make_shared_message<TestMsg>(my_node, uninitialized_handler);
  theMsg->sendMsg(msg.callback_han, new_msg);
}

int main(int argc, char** argv) {
  CollectiveOps::initializeContext(argc, argv);
  CollectiveOps::initializeRuntime();

  HandlerType const callback = theMsg->registerNewHandler(callback_fn);
  my_reinstate_fn = theMsg->registerNewHandler(reinstate_fn);

  my_col_han = theMsg->collectiveRegisterHandler(my_col_fn);

  auto const& my_node = theContext->getNode();
  auto const& num_nodes = theContext->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    TestMsg* msg = new TestMsg(my_node, callback);
    theMsg->broadcastMsg(my_col_han, msg, [=]{ delete msg; });
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
