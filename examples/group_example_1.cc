
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::group;

static GroupType this_group = no_group;

struct HelloMsg : vt::Message {
  int from;

  HelloMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

struct HelloGroupMsg : ::vt::Message {
  HelloGroupMsg() = default;
};

static void hello_world(HelloMsg* msg) {
  fmt::print("{}: Hello from node {}\n", theContext()->getNode(), msg->from);
}

static void hello_group_handler(HelloGroupMsg* msg) {
  fmt::print("{}: Hello from group handler\n", theContext()->getNode());
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    HelloMsg* msg = makeSharedMessage<HelloMsg>(my_node);
    theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);

    //std::vector<region::Region::BoundType> vec{0,1,2,3,4,5,6,7};
    //auto list = std::make_unique<region::List>(vec);
    auto list = std::make_unique<region::Range>(
      theContext()->getNumNodes() / 2,
      theContext()->getNumNodes()
    );
    this_group = theGroup()->newGroup(std::move(list), [](GroupType group){
      fmt::print("Group is created\n");
      auto msg = makeSharedMessage<HelloGroupMsg>();
      envelopeSetGroup(msg->env, group);
      theMsg()->broadcastMsg<HelloGroupMsg, hello_group_handler>(msg);
    });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
