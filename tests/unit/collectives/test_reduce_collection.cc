
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;
using namespace vt::index;
using namespace vt::mapping;

struct MyReduceMsg : ReduceMsg {
  MyReduceMsg(int const& in_num) : num(in_num) {}
  int num = 0;
};

struct VectorPayload {
  VectorPayload() = default;

  friend VectorPayload operator+(VectorPayload v1, VectorPayload const& v2) {
    for (auto&& elm : v2.vec) {
      v1.vec.push_back(elm);
    }
    return v1;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | vec;
  }

  std::vector<double> vec;
};

struct SysMsgVec : ReduceTMsg<VectorPayload> {
  SysMsgVec() = default;
  explicit SysMsgVec(double in_num)
    : ReduceTMsg<VectorPayload>()
  {
    getVal().vec.push_back(in_num);
    getVal().vec.push_back(in_num + 1);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    ReduceTMsg<VectorPayload>::invokeSerialize(s);
  }
};

struct PrintVec {
  void operator()(SysMsgVec* msg) {
    fmt::print("final size={}\n", msg->getConstVal().vec.size());
  }
};

struct MyCol : Collection<MyCol, Index1D> {
  MyCol()
    : Collection<MyCol, Index1D>()
  {
    auto const& node = theContext()->getNode();
    auto const& idx = getIndex();
    fmt::print(
      "{}: constructing MyCol on node={}: idx.x()={}, ptr={}\n",
      node, node, idx.x(), print_ptr(this)
    );
  }

  virtual ~MyCol() = default;
};

struct ColMsg : CollectionMessage<MyCol> {
  NodeType from_node;

  ColMsg() = default;
  ColMsg(NodeType const& in_from_node)
    : CollectionMessage<MyCol>(), from_node(in_from_node)
  { }
};

struct TestReduceCollection : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  virtual void SetUp() {
    TestParallelHarness::SetUp();
  }

  static void colHan(ColMsg* msg, MyCol* col) {
    auto const& node = theContext()->getNode();
    auto const& idx = col->getIndex();
    // fmt::print(
    //   "{}: colHan received: ptr={}, idx={}, getIndex={}\n",
    //   node, print_ptr(col), idx.x(), col->getIndex().x()
    // );

    auto reduce_msg = makeSharedMessage<MyReduceMsg>(idx.x());
    auto proxy = col->getProxy();
    // fmt::print("reduce_msg->num={}\n", reduce_msg->num);
    theCollection()->reduceMsg<MyCol,MyReduceMsg,reducePlus>(proxy, reduce_msg);
  }

  static void colHanPartial(ColMsg* msg, MyCol* col) {
    auto const& node = theContext()->getNode();
    auto const& idx = col->getIndex();
    // fmt::print(
    //   "{}: colHan received: ptr={}, idx={}, getIndex={}\n",
    //   node, print_ptr(col), idx.x(), col->getIndex().x()
    // );

    auto reduce_msg = makeSharedMessage<MyReduceMsg>(idx.x());
    auto proxy = col->getProxy();
    //fmt::print("reduce_msg->num={}\n", reduce_msg->num);
    theCollection()->reduceMsgExpr<MyCol,MyReduceMsg,reducePlus>(
      proxy,reduce_msg, [](Index1D const& idx) -> bool {
        return idx.x() < 8;
      }
    );
  }

  static void colHanPartialProxy(ColMsg* msg, MyCol* col) {
    auto const& node = theContext()->getNode();
    auto const& idx = col->getIndex();
    // fmt::print(
    //   "{}: colHan received: ptr={}, idx={}, getIndex={}\n",
    //   node, print_ptr(col), idx.x(), col->getIndex().x()
    // );

    auto reduce_msg = makeSharedMessage<MyReduceMsg>(idx.x());
    auto proxy = col->getCollectionProxy();
    //fmt::print("reduce_msg->num={}\n", reduce_msg->num);
    proxy.reduceExpr<MyReduceMsg,reducePlus>(
      reduce_msg, [](Index1D const& idx) -> bool {
        return idx.x() < 8;
      }
    );
  }

  static void colHanVec(ColMsg* msg, MyCol* col) {
    auto const& node = theContext()->getNode();
    auto const& idx = col->getIndex();
    // fmt::print(
    //   "{}: colHanVec received: ptr={}, idx={}, getIndex={}\n",
    //   node, print_ptr(col), idx.x(), col->getIndex().x()
    // );

    auto reduce_msg = makeSharedMessage<SysMsgVec>(static_cast<double>(idx.x()));
    auto proxy = col->getProxy();
    // fmt::print(
    //   "reduce_msg->vec.size()={}\n", reduce_msg->getConstVal().vec.size()
    // );
    theCollection()->reduceMsg<
      MyCol,
      SysMsgVec,
      SysMsgVec::msgHandler<SysMsgVec,PlusOp<VectorPayload>,PrintVec>
    >(proxy, reduce_msg);
  }

  static void colHanVecProxy(ColMsg* msg, MyCol* col) {
    auto const& node = theContext()->getNode();
    auto const& idx = col->getIndex();
    // fmt::print(
    //   "{}: colHanVec received: ptr={}, idx={}, getIndex={}\n",
    //   node, print_ptr(col), idx.x(), col->getIndex().x()
    // );

    auto reduce_msg = makeSharedMessage<SysMsgVec>(static_cast<double>(idx.x()));
    auto proxy = col->getCollectionProxy();
    // fmt::print(
    //   "reduce_msg->vec.size()={}\n", reduce_msg->getConstVal().vec.size()
    // );
    proxy.reduce<
      SysMsgVec,
      SysMsgVec::msgHandler<SysMsgVec,PlusOp<VectorPayload>,PrintVec>
    >(reduce_msg);
  }

  static void reducePlus(MyReduceMsg* msg) {
    fmt::print(
      "{}: cur={}: is_root={}, count={}, next={}, num={}\n",
      theContext()->getNode(), print_ptr(msg), print_bool(msg->isRoot()),
      msg->getCount(), print_ptr(msg->getNext<MyReduceMsg>()), msg->num
    );

    if (msg->isRoot()) {
      fmt::print("{}: final num={}\n", theContext()->getNode(), msg->num);
    } else {
      MyReduceMsg* fst_msg = msg;
      MyReduceMsg* cur_msg = msg->getNext<MyReduceMsg>();
      while (cur_msg != nullptr) {
        fmt::print(
          "{}: while fst_msg={}: cur_msg={}, is_root={}, count={}, next={}, num={}\n",
          theContext()->getNode(),
          print_ptr(fst_msg), print_ptr(cur_msg), print_bool(cur_msg->isRoot()),
          cur_msg->getCount(), print_ptr(cur_msg->getNext<MyReduceMsg>()),
          cur_msg->num
        );

        fst_msg->num += cur_msg->num;
        cur_msg = cur_msg->getNext<MyReduceMsg>();
      }
    }
  }
};

TEST_F(TestReduceCollection, test_reduce_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& range = Index1D(32);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(this_node);
    proxy.broadcast<ColMsg,colHan>(msg);
  }
}

TEST_F(TestReduceCollection, test_reduce_partial_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& range = Index1D(32);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(this_node);
    proxy.broadcast<ColMsg,colHanPartial>(msg);
  }
}

TEST_F(TestReduceCollection, test_reduce_vec_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& range = Index1D(32);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(this_node);
    proxy.broadcast<ColMsg,colHanVec>(msg);
  }
}

TEST_F(TestReduceCollection, test_reduce_partial_proxy_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& range = Index1D(32);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(this_node);
    proxy.broadcast<ColMsg,colHanPartialProxy>(msg);
  }
}

TEST_F(TestReduceCollection, test_reduce_vec_proxy_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& range = Index1D(32);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(this_node);
    proxy.broadcast<ColMsg,colHanVecProxy>(msg);
  }
}

}}} // end namespace vt::tests::unit
