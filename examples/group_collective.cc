
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

  if (num_nodes < 2) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  srand48(my_node * 29);

  auto const& random_node_filter = drand48() < 0.5;
  auto const& even_node_filter   = my_node % 2 == 0;
  auto const& odd_node_filter    = my_node % 2 == 1;

  this_group = theGroup()->newGroupCollective(
    odd_node_filter, [=](GroupType group){
      auto const& root = 0;
      auto const& in_group = theGroup()->inGroup(group);
      auto const& root_node = theGroup()->groupRoot(group);
      auto const& is_default_group = theGroup()->groupDefault(group);
      fmt::print(
        "{}: Group is created: group={}, in_group={}, root={}, "
        "is_default_group={}\n",
        my_node, group, in_group, root_node, is_default_group
      );
      if (in_group) {
        auto msg = makeSharedMessage<SysMsg>(1);
        //fmt::print("msg->num={}\n", msg->getConstVal());
        theGroup()->groupReduce(group)->reduce<
          SysMsg, SysMsg::msgHandler<SysMsg,collective::PlusOp<int>,Print>
        >(root, msg);
      }
      fmt::print("node={}\n", my_node);
      if (my_node == 1) {
        //assert(in_group);
        auto msg = makeSharedMessage<HelloGroupMsg>();
        envelopeSetGroup(msg->env, group);
        fmt::print("calling broadcasting={}\n", my_node);
        theMsg()->broadcastMsg<HelloGroupMsg, hello_group_handler>(msg);
      }
    }
  );

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
