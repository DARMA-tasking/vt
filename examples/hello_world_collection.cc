
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::index;
using namespace vt::mapping;

struct MyCol : Collection<MyCol, Index1D> {
  Index1D idx;
  double test_val = 0.0;

  MyCol(Index1D in_idx)
    : Collection<MyCol, Index1D>(), idx(in_idx)
  {
    auto const& node = theContext()->getNode();
    fmt::print(
      "{}: constructing MyCol on node={}: idx.x()={}, ptr={}\n",
      node, node, idx.x(), print_ptr(this)
    );
  }
  MyCol() = default;

  virtual ~MyCol() {
    auto const& node = theContext()->getNode();
    fmt::print(
      "{}: invoking destructor MyCol on node={}: idx.x()={}, ptr={}\n",
      node, node, idx.x(), print_ptr(this)
    );
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    Collection<MyCol, Index1D>::serialize(s);
    s | idx;
    s | test_val;
  }
};

struct OtherColl : Collection<OtherColl, Index2D> {
  Index2D idx;

  OtherColl() = default;
  OtherColl(Index2D in_idx)
    : Collection<OtherColl, Index2D>(), idx(in_idx)
  {
    auto const& node = theContext()->getNode();
    fmt::print(
      "{}: constructing OtherColl on node={}: idx=[{},{}]\n",
      node, node, idx.x(), idx.y()
    );
  }

  virtual ~OtherColl() {
    auto const& node = theContext()->getNode();
    fmt::print(
      "{}: invoking destructor OtherColl on node={}: idx=[{},{}]\n",
      node, node, idx.x(), idx.y()
    );
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    Collection<OtherColl, Index2D>::serialize(s);
    s | idx;
  }
};

template <typename ColT>
struct ColMsg : CollectionMessage<ColT> {
  NodeType from_node;
  int val_ = 0;

  ColMsg() = default;
  explicit ColMsg(NodeType const& in_from_node)
    : CollectionMessage<ColT>(), from_node(in_from_node)
  { }
  ColMsg(NodeType const& in_from_node, int32_t const val)
    : CollectionMessage<ColT>(), from_node(in_from_node), val_(val)
  { }
};

static void colHan2(ColMsg<MyCol>* msg, MyCol* col) {
  auto const& node = theContext()->getNode();
  fmt::print(
    "{}: colHan2 received: ptr={}, idx={}, getIndex={}\n",
    node, print_ptr(col), col->idx.x(), col->getIndex().x()
  );
}

static void method1(ColMsg<MyCol>* msg, MyCol* col) {
  auto const& node = theContext()->getNode();
  fmt::print(
    "{}: method1 received: ptr={}, idx={}, getIndex={}\n",
    node, print_ptr(col), col->idx.x(), col->getIndex().x()
  );
  double val = 0.1f;
  double val2 = 0.4f * msg->val_;
  int x = col->getIndex().x() < 8 ? 10000 : (col->getIndex().x() > 40 ? 1000 : 10);
  for (int i = 0; i < 10000 * x; i++) {
    val *= val2 + i*29.4;
    val2 += 1.0;
  }
  col->test_val += val + val2;
}

static void colHan(ColMsg<MyCol>* msg, MyCol* col) {
  auto const& node = theContext()->getNode();
  fmt::print(
    "{}: colHan received: ptr={}, idx={}, getIndex={}\n",
    node, print_ptr(col), col->idx.x(), col->getIndex().x()
  );

  #define TEST_MIGRATE 0
  #if TEST_MIGRATE
  if (col->idx.x() == 2) {
    auto const& this_node = theContext()->getNode();
    auto const& num_nodes = theContext()->getNumNodes();
    auto const& next_node = this_node + 1;
    fmt::print(
      "{}: colHan calling migrate: idx={}\n", node, col->idx.x()
    );
    col->migrate(next_node >= num_nodes ? 0 : next_node);
  }
  #endif
}

static void colHanOther(ColMsg<OtherColl>* msg, OtherColl* col) {
  auto const& node = theContext()->getNode();
  fmt::print(
    "{}: colHanOther received: idx=[{},{}]\n",
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
  fmt::print("{}: Hello from node {}\n", theContext()->getNode(), msg->from);
}

static constexpr int32_t const default_num_elms = 128;

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

  #if 1
  #if LB_ENABLED
    EpochType const epoch = theTerm()->newEpoch();
    theMsg()->setGlobalEpoch(epoch);
  #endif

  if (my_node == 0) {
    auto const& this_node = theContext()->getNode();
    auto const& range = Index1D(num_elms);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg<MyCol>(this_node);

    #if !LB_ENABLED
      for (int i = 0; i < num_elms; i++) {
        auto msg = new ColMsg<MyCol>(this_node);
        proxy[i].send<ColMsg<MyCol>,colHan>(msg);
        proxy[i].send<ColMsg<MyCol>,colHan2>(msg);
      }
    #endif

    proxy.broadcast<ColMsg<MyCol>,colHan>(msg);

    #if LB_ENABLED
      for (int i = 0; i < num_elms; i++) {
        auto msg = new ColMsg<MyCol>(this_node, i);
        proxy[i].send<ColMsg<MyCol>,method1>(msg);
      }
      theTerm()->addEpochAction(epoch,[=]{
        theCollection()->nextPhase<MyCol>(proxy,0);
      });
    #endif
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
        auto msg = new ColMsg<OtherColl>(this_node);
        theCollection()->sendMsg<ColMsg<OtherColl>, colHanOther>(
          proxy(i,j), msg, nullptr
        );
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
