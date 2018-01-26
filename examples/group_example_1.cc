
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

struct HelloGroupMsg : GroupUserMsg<Message> {
  HelloGroupMsg() = default;
};

static void hello_world(HelloMsg* msg) {
  printf("%d: Hello from node %d\n", theContext()->getNode(), msg->from);
}

static void hello_group_handler(HelloGroupMsg* msg) {
  printf("%d: Hello from group handler\n", theContext()->getNode());
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    HelloMsg* msg = new HelloMsg(my_node);
    theMsg()->broadcastMsg<HelloMsg, hello_world>(msg, [=]{ delete msg; });

    std::vector<region::Region::BoundType> vec{0,1,2,3,4,5,6,7};
    auto list = std::make_unique<region::List>(vec);
    this_group = theGroup()->newGroup(std::move(list), []{
      printf("Group is created\n");
      auto msg = makeSharedMessage<HelloGroupMsg>();
      theGroup()->sendMsg<HelloGroupMsg, hello_group_handler>(this_group, msg);
    });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
