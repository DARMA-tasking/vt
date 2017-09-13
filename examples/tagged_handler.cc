
#include "transport.h"
#include <cstdlib>

using namespace vt;

static HandlerType my_reinstate_fn = uninitialized_handler;
static TagType const first_recv_tag = 10;
static TagType const last_recv_tag = 15;
static TagType cur_iter = first_recv_tag;
static int count = 0;

struct TestMsg : vt::Message {
  NodeType from;
  HandlerType callback_han;

  TestMsg(NodeType const& in_from, HandlerType const& in_callback_han)
    : Message(), from(in_from), callback_han(in_callback_han)
  { }
};

static void process_iter_msgs(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  auto const& first_tag = envelopeGetTag(msg.env);

  count++;

  printf(
    "%d: process iteration node %d: count=%d, tag=%d, iteration=%d\n",
    theContext->getNode(), msg.from, count, first_tag, cur_iter
  );

  assert(first_tag == cur_iter);

  // received all for this iteration
  if (count == theContext->getNumNodes() - 1) {
    cur_iter++;
    count = 0;

    auto const& first_han = theMsg->getCurrentHandler();
    theMsg->unregisterHandlerFn(first_han, cur_iter-1);
    theMsg->registerHandlerFn(first_han, process_iter_msgs, cur_iter);

    printf(
      "%d: updating to NEXT iteration node %d: count=%d, cur_iter=%d\n",
      theContext->getNode(), msg.from, count, cur_iter
    );
  }
}

static void my_col_fn(TestMsg* msg) {
  auto const& my_node = theContext->getNode();

  printf(
    "%d: my_col_fn from=%d, callback=%d: tag=%d, sending, tag=[%d,%d]\n",
    my_node, msg->from, msg->callback_han, first_recv_tag, first_recv_tag, last_recv_tag
  );

  for (auto i = first_recv_tag; i < last_recv_tag; i++) {
    TestMsg* new_msg = make_shared_message<TestMsg>(my_node, uninitialized_handler);
    theMsg->sendMsg(msg->callback_han, new_msg, i);
    message_deref(new_msg);
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initializeContext(argc, argv);
  CollectiveOps::initializeRuntime();

  HandlerType const callback = theMsg->registerNewHandler(
    process_iter_msgs, cur_iter
  );

  auto const& my_node = theContext->getNode();
  auto const& num_nodes = theContext->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    TestMsg* msg = make_shared_message<TestMsg>(my_node, callback);
    theMsg->broadcastMsg<TestMsg, my_col_fn>(msg);
    message_deref(msg);
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
