
#include "vt/transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

struct TestMsg : CallbackMessage {
  NodeType from;

  TestMsg(NodeType const& in_from) : CallbackMessage(), from(in_from) { }
};

static void test_msg_recv(TestMsg* msg) {
  fmt::print("{}: sending callback {}\n", theContext()->getNode(), msg->from);

  TestMsg* sendMsg = makeSharedMessage<TestMsg>(my_node);
  theMsg()->sendCallback(sendMsg);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    for (int cur_node = 0; cur_node < num_nodes; cur_node++) {
      TestMsg* msg = makeSharedMessage<TestMsg>(my_node);
      theMsg()->sendDataCallback<TestMsg, test_msg_recv>(
        cur_node, msg, [=](BaseMessage* in_msg){
          TestMsg* msg = static_cast<TestMsg*>(in_msg);
          fmt::print("{}: callback received from {}\n", theContext()->getNode(), msg->from);
        }
      );
    }
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
