
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::vrt;
using namespace vt::vrt::collection;
using namespace vt::index;
using namespace vt::mapping;

struct MyCol : Collection<Index1D> {
  Index1D idx;

  MyCol(Index1D in_idx) : Collection<Index1D>()
  {
    idx = in_idx;
    auto const& node = theContext()->getNode();
    printf("constructing MyCol on node=%d: idx.x()=%d\n", node, idx.x());
  }
};

struct ColMsg : CollectionMessage<MyCol::IndexType> {
  NodeType from_node;

  ColMsg() = default;

  explicit ColMsg(NodeType const& in_from_node)
    : CollectionMessage(), from_node(in_from_node)
  { }
};

static void colHan(ColMsg* msg, MyCol* col) {
  printf("colHan received: idx=%d\n", col->idx.x());
}

struct HelloMsg : vt::Message {
  int from;

  HelloMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

static void hello_world(HelloMsg* msg) {
  printf("%d: Hello from node %d\n", theContext()->getNode(), msg->from);
}

static constexpr int32_t const default_num_elms = 4;

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  if (my_node == 0) {
    auto const& range = Index1D(num_elms);
    auto proxy = theCollection()->construct<MyCol>(range);
    for (int i = 10; i < 40; i++) {
      auto const& this_node = theContext()->getNode();
      auto msg = new ColMsg(this_node);
      theCollection()->sendMsg<MyCol, ColMsg, colHan>(proxy[i], msg, nullptr);
    }
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
