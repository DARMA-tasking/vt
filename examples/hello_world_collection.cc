
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::vrt;
using namespace vt::vrt::collection;
using namespace vt::index;
using namespace vt::mapping;

struct MyCol : Collection<Index1D> {
  Index1D idx;

  MyCol(Index1D in_idx)
    : Collection<Index1D>(), idx(in_idx)
  {
    auto const& node = theContext()->getNode();
    printf(
      "%d: constructing MyCol on node=%d: idx.x()=%d\n",
      node, node, idx.x()
    );
  }
};

struct OtherColl : Collection<Index2D> {
  Index2D idx;

  OtherColl(Index2D in_idx)
    : Collection<Index2D>(), idx(in_idx)
  {
    auto const& node = theContext()->getNode();
    printf(
      "%d: constructing OtherColl on node=%d: idx={%d,%d}\n",
      node, node, idx.x(), idx.y()
    );
  }
};

template <typename IndexT>
struct ColMsg : CollectionMessage<IndexT> {
  NodeType from_node;

  ColMsg() = default;
  ColMsg(NodeType const& in_from_node)
    : CollectionMessage<IndexT>(), from_node(in_from_node)
  { }
};

static void colHan(ColMsg<Index1D>* msg, MyCol* col) {
  auto const& node = theContext()->getNode();
  printf(
    "%d: colHan received: idx=%d\n", node, col->idx.x()
  );
}

static void colHanOther(ColMsg<Index2D>* msg, OtherColl* col) {
  auto const& node = theContext()->getNode();
  printf(
    "%d: colHanOther received: idx={%d,%d}\n",
    node, col->idx.x(), col->idx.y()
  );
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

static constexpr int32_t const default_num_elms = 25;

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    //CollectiveOps::abort("At least 2 ranks required");
  }

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  #if 1
  if (my_node == 0) {
    auto const& range = Index1D(num_elms);
    auto proxy = theCollection()->construct<MyCol>(range);
    for (int i = 0; i < num_elms; i++) {
      auto const& this_node = theContext()->getNode();
      auto msg = new ColMsg<Index1D>(this_node);
      theCollection()->sendMsg<MyCol, ColMsg<Index1D>, colHan>(
        proxy[i], msg, nullptr
      );
    }
  }
  #endif
  #if 0
  if (my_node == 1) {
    auto const& dim1 = num_elms/2;
    auto const& dim2 = (num_elms+2)/2;
    auto const& range_2d = Index2D(dim1,dim2);
    auto proxy = theCollection()->construct<OtherColl>(range_2d);
    for (int i = 0; i < dim1; i++) {
      for (int j = 0; j < dim2; j++) {
        auto const& this_node = theContext()->getNode();
        auto msg = new ColMsg<Index2D>(this_node);
        theCollection()->sendMsg<
          OtherColl, ColMsg<Index2D>, colHanOther
        >(proxy(i,j), msg, nullptr);
      }
    }
  }
  #endif

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
