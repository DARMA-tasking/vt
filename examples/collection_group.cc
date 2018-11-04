
#include "vt/transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::group;

static GroupType this_group = no_group;

struct TestMsg;

struct MyReduceMsg : collective::ReduceMsg {
  MyReduceMsg(int const& in_num) : num(in_num) {}
  int num = 0;
};

struct ColA : Collection<ColA,Index1D> {
  ColA()
    : Collection<ColA,Index1D>()
  {
    // auto const& this_node = theContext()->getNode();
    // ::fmt::print("{}: constructing: idx={}\n", this_node, getIndex().x());
  }

  void work(TestMsg* msg);
  void work2(TestMsg* msg);
};

struct TestMsg : CollectionMessage<ColA> { };

void ColA::work2(TestMsg* msg) {
  auto const& this_node = theContext()->getNode();
  ::fmt::print("work2: node={}, idx={}\n", this_node, getIndex().x());
}

static void reduceNone(MyReduceMsg* msg) {
  if (msg->isRoot()) {
    fmt::print("{}: at root: final num={}\n", theContext()->getNode(), msg->num);
  } else {
    MyReduceMsg* fst_msg = msg;
    MyReduceMsg* cur_msg = msg->getNext<MyReduceMsg>();
    while (cur_msg != nullptr) {
      // fmt::print(
      //   "{}: while fst_msg={}: cur_msg={}, is_root={}, count={}, next={}, num={}\n",
      //   theContext()->getNode(),
      //   print_ptr(fst_msg), print_ptr(cur_msg), print_bool(cur_msg->isRoot()),
      //   cur_msg->getCount(), print_ptr(cur_msg->getNext<MyReduceMsg>()),
      //   cur_msg->num
      // );
      fst_msg->num += cur_msg->num;
      cur_msg = cur_msg->getNext<MyReduceMsg>();
    }
  }
}

void ColA::work(TestMsg* msg) {
  auto const& this_node = theContext()->getNode();
  ::fmt::print("work: node={}, idx={}\n", this_node, getIndex().x());

  if (getIndex().x() == 2) {
    auto const& proxy = getCollectionProxy();
    auto const& msg = makeSharedMessage<TestMsg>();
    proxy.broadcast<TestMsg,&ColA::work2>(msg);
  }

  auto reduce_msg = makeSharedMessage<MyReduceMsg>(getIndex().x());
  auto const& proxy = getCollectionProxy();
  theCollection()->reduceMsg<ColA,MyReduceMsg,reduceNone>(proxy, reduce_msg);
}

struct HelloMsg : vt::Message {
  int from;

  explicit HelloMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

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

  if (my_node == 0) {
    auto const& range = Index1D(std::max(num_nodes / 2, 1));
    //auto const& range = Index1D(static_cast<int>(num_nodes));
    auto const& proxy = theCollection()->construct<ColA>(range);
    auto const& msg = makeSharedMessage<TestMsg>();
    proxy.broadcast<TestMsg,&ColA::work>(msg);
    // proxy.broadcast<TestMsg,&ColA::work>(msg);
    // proxy.broadcast<TestMsg,&ColA::work>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
