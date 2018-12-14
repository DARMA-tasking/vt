
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct CallbackMsg : vt::Message {
  CallbackMsg() = default;
  explicit CallbackMsg(Callback<> in_cb) : cb_(in_cb) { }

  Callback<> cb_;
};

static int32_t called = 0;

struct TestCallbackFunc : TestParallelHarness {
  static void test_handler(CallbackMsg* msg) {
    auto const& this_node = theContext()->getNode();
    //fmt::print("{}: test_handler\n", this_node);
    msg->cb_.send();
  }
};

TEST_F(TestCallbackFunc, test_callback_func_1) {
  called = 0;
  auto cb = theCB()->makeFunc([]{ called = 900; });
  cb.send();
  EXPECT_EQ(called, 900);
}

TEST_F(TestCallbackFunc, test_callback_func_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  called = 0;

  if (this_node == 0) {
    auto cb = theCB()->makeFunc([]{ called = 400; });
    auto msg = makeSharedMessage<CallbackMsg>(cb);
    theMsg()->sendMsg<CallbackMsg, test_handler>(1, msg);

    theTerm()->addAction([=]{
      EXPECT_EQ(called, 400);
    });
  }
}

}}} // end namespace vt::tests::unit
