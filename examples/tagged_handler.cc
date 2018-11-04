
#include "vt/transport.h"
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

static void processIterMsgs(vt::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  auto const& first_tag = envelopeGetTag(msg.env);

  count++;

  fmt::print(
    "{}: process iteration node {}: count={}, tag={}, iteration={}\n",
    theContext()->getNode(), msg.from, count, first_tag, cur_iter
  );

  assert(first_tag == cur_iter);

  // received all for this iteration
  if (count == theContext()->getNumNodes() - 1) {
    cur_iter++;
    count = 0;

    auto const& first_han = theMsg()->getCurrentHandler();
    theMsg()->unregisterHandlerFn(first_han, cur_iter-1);
    theMsg()->registerHandlerFn(first_han, processIterMsgs, cur_iter);

    fmt::print(
      "{}: updating to NEXT iteration node {}: count={}, cur_iter={}\n",
      theContext()->getNode(), msg.from, count, cur_iter
    );
  }
}

static void myColFn(TestMsg* msg) {
  auto const& my_node = theContext()->getNode();

  fmt::print(
    "{}: my_col_fn from={}, callback={}: tag={}, sending, tag=[{},{}]\n",
    my_node, msg->from, msg->callback_han, first_recv_tag, first_recv_tag, last_recv_tag
  );

  for (auto i = first_recv_tag; i < last_recv_tag; i++) {
    TestMsg* new_msg = makeSharedMessage<TestMsg>(my_node, uninitialized_handler);
    theMsg()->sendMsg(msg->callback_han, new_msg, i);
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  HandlerType const callback = theMsg()->registerNewHandler(
    processIterMsgs, cur_iter
  );

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    TestMsg* msg = makeSharedMessage<TestMsg>(my_node, callback);
    theMsg()->broadcastMsg<TestMsg, myColFn>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
