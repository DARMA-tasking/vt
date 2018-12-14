
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

#include <memory>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct CallbackMsg : vt::Message {
  CallbackMsg() = default;
  explicit CallbackMsg(Callback<> in_cb) : cb_(in_cb) { }

  Callback<> cb_;
};

struct DataMsg : vt::Message {
  DataMsg() = default;
  DataMsg(int in_a, int in_b, int in_c) : a(in_a), b(in_b), c(in_c) { }
  int a = 0, b = 0, c = 0;
};

struct CallbackDataMsg : vt::Message {
  CallbackDataMsg() = default;
  explicit CallbackDataMsg(Callback<DataMsg> in_cb) : cb_(in_cb) { }

  Callback<DataMsg> cb_;
};

static int32_t called = 0;

struct TestCallbackBcastCollection : TestParallelHarness {
  static void testHandler(CallbackDataMsg* msg) {
    auto const& n = theContext()->getNode();
    auto nmsg = makeSharedMessage<DataMsg>(8,9,10);
    msg->cb_.send(nmsg);
  }
  static void testHandlerEmpty(CallbackMsg* msg) {
    auto const& n = theContext()->getNode();
    msg->cb_.send();
  }
};

struct TestCol : vt::Collection<TestCol, vt::Index1D> {
  TestCol() = default;

  virtual ~TestCol() {
    // fmt::print(
    //   "{}: destroying {}: val={}\n",
    //   theContext()->getNode(), this->getIndex(), val
    // );
    if (other) {
      EXPECT_EQ(val, 29);
    } else {
      EXPECT_EQ(val, 13);
    }
  }

  void cb1(DataMsg* msg) {
    EXPECT_EQ(msg->a, 8);
    EXPECT_EQ(msg->b, 9);
    EXPECT_EQ(msg->c, 10);
    val = 29;
    other = true;
  }

  void cb2(DataMsg* msg) {
    EXPECT_EQ(msg->a, 8);
    EXPECT_EQ(msg->b, 9);
    EXPECT_EQ(msg->c, 10);
    val = 13;
  }

public:
  int32_t val = 17;
  bool other = false;
};

static void cb3(DataMsg* msg, TestCol* col) {
  EXPECT_EQ(msg->a, 8);
  EXPECT_EQ(msg->b, 9);
  EXPECT_EQ(msg->c, 10);
  col->val = 13;
}

TEST_F(TestCallbackBcastCollection, test_callback_bcast_collection_1) {
  auto const& this_node = theContext()->getNode();

  if (this_node == 0) {
    auto const& range = Index1D(32);
    auto proxy = theCollection()->construct<TestCol>(range);
    auto cb = theCB()->makeBcast<TestCol,DataMsg,&TestCol::cb1>(proxy);
    auto nmsg = makeSharedMessage<DataMsg>(8,9,10);
    cb.send(nmsg);

    theTerm()->addAction([=]{
      proxy.destroy();
    });
  }
}

TEST_F(TestCallbackBcastCollection, test_callback_bcast_collection_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  if (this_node == 0) {
    auto const& range = Index1D(32);
    auto proxy = theCollection()->construct<TestCol>(range);
    auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<TestCol,DataMsg,&TestCol::cb2>(proxy);
    auto msg = makeSharedMessage<CallbackDataMsg>(cb);
    theMsg()->sendMsg<CallbackDataMsg, testHandler>(next, msg);

    theTerm()->addAction([=]{
      proxy.destroy();
    });
  }
}

TEST_F(TestCallbackBcastCollection, test_callback_bcast_collection_3) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  if (this_node == 0) {
    auto const& range = Index1D(32);
    auto proxy = theCollection()->construct<TestCol>(range);
    auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<TestCol,DataMsg,cb3>(proxy);
    auto msg = makeSharedMessage<CallbackDataMsg>(cb);
    theMsg()->sendMsg<CallbackDataMsg, testHandler>(next, msg);

    theTerm()->addAction([=]{
      proxy.destroy();
    });
  }
}

}}} // end namespace vt::tests::unit
