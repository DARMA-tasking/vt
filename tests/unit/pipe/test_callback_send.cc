
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

struct TestCallbackSend : TestParallelHarness {
  static void testHandler(CallbackDataMsg* msg) {
    auto const& n = theContext()->getNode();
    auto nmsg = makeSharedMessage<DataMsg>(1,2,3);
    msg->cb_.send(nmsg);
  }
  static void testHandlerEmpty(CallbackMsg* msg) {
    auto const& n = theContext()->getNode();
    msg->cb_.send();
  }
};

static void callbackFn(DataMsg* msg) {
  EXPECT_EQ(msg->a, 1);
  EXPECT_EQ(msg->b, 2);
  EXPECT_EQ(msg->c, 3);
  called = 100;
}

struct CallbackFunctor {
  void operator()(DataMsg* msg) {
    EXPECT_EQ(msg->a, 1);
    EXPECT_EQ(msg->b, 2);
    EXPECT_EQ(msg->c, 3);
    called = 200;
  }
};

struct CallbackFunctorEmpty {
  void operator()() {
    called = 300;
  }
};

TEST_F(TestCallbackSend, test_callback_send_1) {
  auto const& this_node = theContext()->getNode();

  called = 0;
  auto cb = theCB()->makeSend<DataMsg,callbackFn>(this_node);
  auto nmsg = makeSharedMessage<DataMsg>(1,2,3);
  cb.send(nmsg);

  theTerm()->addAction([=]{
    EXPECT_EQ(called, 100);
  });
}

TEST_F(TestCallbackSend, test_callback_send_2) {
  auto const& this_node = theContext()->getNode();
  called = 0;
  auto cb = theCB()->makeSend<CallbackFunctor>(this_node);
  auto nmsg = makeSharedMessage<DataMsg>(1,2,3);
  cb.send(nmsg);

  theTerm()->addAction([=]{
    EXPECT_EQ(called, 200);
  });
}

TEST_F(TestCallbackSend, test_callback_send_3) {
  auto const& this_node = theContext()->getNode();
  called = 0;
  auto cb = theCB()->makeSend<CallbackFunctorEmpty>(this_node);
  cb.send();

  theTerm()->addAction([=]{
    EXPECT_EQ(called, 300);
  });
}

TEST_F(TestCallbackSend, test_callback_send_remote_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;
  auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
  auto cb = theCB()->makeSend<DataMsg,callbackFn>(this_node);
  auto msg = makeSharedMessage<CallbackDataMsg>(cb);
  theMsg()->sendMsg<CallbackDataMsg, testHandler>(next, msg);

  theTerm()->addAction([=]{
    EXPECT_EQ(called, 100);
  });
}

TEST_F(TestCallbackSend, test_callback_send_remote_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;
  auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
  auto cb = theCB()->makeSend<CallbackFunctor>(this_node);
  auto msg = makeSharedMessage<CallbackDataMsg>(cb);
  theMsg()->sendMsg<CallbackDataMsg, testHandler>(next, msg);

  theTerm()->addAction([=]{
    EXPECT_EQ(called, 200);
  });
}

TEST_F(TestCallbackSend, test_callback_send_remote_3) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;
  auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
  auto cb = theCB()->makeSend<CallbackFunctorEmpty>(this_node);
  auto msg = makeSharedMessage<CallbackMsg>(cb);
  theMsg()->sendMsg<CallbackMsg, testHandlerEmpty>(next, msg);

  theTerm()->addAction([=]{
    EXPECT_EQ(called, 300);
  });
}


}}} // end namespace vt::tests::unit
