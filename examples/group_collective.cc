
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::group;

static GroupType this_group = no_group;

struct HelloMsg : vt::Message {
  int from;

  explicit HelloMsg(int const& in_from)
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

struct SysMsg : collective::ReduceTMsg<int> {
  explicit SysMsg(int in_num)
    : ReduceTMsg<int>(in_num)
  { }
};

struct Print {
  void operator()(SysMsg* msg) {
    fmt::print("final value={}\n", msg->getConstVal());
  }
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  this_group = theGroup()->newGroupCollective(
    my_node < num_nodes / 2, [=](GroupType group){
      auto const& root = 0;
      auto const& in_group = theGroup()->inGroup(group);
      fmt::print(
        "{}: Group is created: group={}, in_group={}\n",
        my_node, group, in_group
      );
      if (in_group) {
        auto msg = makeSharedMessage<SysMsg>(1);
        //fmt::print("msg->num={}\n", msg->getConstVal());
        theGroup()->groupReduce(group)->reduce<
          SysMsg, SysMsg::msgHandler<SysMsg,collective::PlusOp<int>,Print>
        >(root, msg);
      }
    }
  );
  // if (my_node == 0) {
  //   HelloMsg* msg = makeSharedMessage<HelloMsg>(my_node);
  //   theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);

  //   //std::vector<region::Region::BoundType> vec{0,1,2,3,4,5,6,7};
  //   //auto list = std::make_unique<region::List>(vec);
  //   auto list = std::make_unique<region::Range>(
  //     theContext()->getNumNodes() / 2,
  //     theContext()->getNumNodes()
  //   );
  // }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
