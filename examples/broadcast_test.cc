
#include "transport.h"
#include <cstdlib>

using namespace vt;

#define BCAST_DEBUG 0

static constexpr int32_t const num_bcasts = 4;
static NodeType my_node = uninitialized_destination;
static int32_t count = 0;

struct Msg : vt::Message {
  NodeType broot;

  Msg(NodeType const& in_broot) : Message(), broot(in_broot) { }
};

static void bcastTest(Msg* msg) {
  auto const& root = msg->broot;

  #if BCAST_DEBUG
  printf("%d: bcastTestHandler root=%d\n", theContext()->getNode(), msg->broot);
  #endif

  assert(
    root != my_node and "Broadcast should deliver to all but this node"
  );

  count++;
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();

  if (theContext()->getNumNodes() == 1) {
    CollectiveOps::abort("At least 2 ranks required");


  }

  NodeType from_node = uninitialized_destination;

  if (argc > 1) {
    from_node = atoi(argv[1]);
  }

  int32_t const expected = num_bcasts *
    (from_node == uninitialized_destination ? theContext()->getNumNodes() - 1 : (
      from_node == my_node ? 0 : 1
    ));

  theTerm()->attachGlobalTermAction([=]{
    printf("[%d] verify: count=%d, expected=%d\n", my_node, count, expected);
    assert(count == expected);
  });

  if (from_node == uninitialized_destination or from_node == my_node) {
    printf("[%d] broadcast_test: broadcasting %d times\n", my_node, num_bcasts);
    for (int i = 0; i < num_bcasts; i++) {
      theMsg()->broadcastMsg<Msg, bcastTest>(makeSharedMessage<Msg>(my_node));
    }
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
