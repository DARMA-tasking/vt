
#include "transport.h"

#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;
static EpochType cur_epoch = no_epoch;
static int32_t num = 2;

struct TestMsg : Message {};

static void test_handler(TestMsg* msg) {
  ::fmt::print("node={}, running handler: num={}\n", my_node, num);
  num--;

  if (num > 0) {
    auto msg = makeSharedMessage<TestMsg>();
    envelopeSetEpoch(msg->env, cur_epoch);
    theMsg()->sendMsg<TestMsg,test_handler>((my_node + 1) % num_nodes,msg);
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (0) {
    cur_epoch = theTerm()->newEpoch();

    fmt::print("{}: new cur_epoch={}\n", my_node, cur_epoch);

    theMsg()->sendMsg<TestMsg,test_handler>(
      (my_node + 1) % num_nodes,
      makeSharedMessage<TestMsg>()
    );

    theTerm()->addAction(cur_epoch, []{
      fmt::print("{}: running attached action: cur_epoch={}\n", my_node, cur_epoch);
    });
    theTerm()->finishedEpoch(cur_epoch);
  }

  if (my_node == 0) {
    cur_epoch = theTerm()->newEpochRooted(true);

    theTerm()->addAction(cur_epoch, []{
      fmt::print("{}: running attached action: cur_epoch={}\n", my_node, cur_epoch);
    });

    auto msg = makeSharedMessage<TestMsg>();
    envelopeSetEpoch(msg->env, cur_epoch);
    theMsg()->sendMsg<TestMsg,test_handler>((my_node + 1) % num_nodes,msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}

