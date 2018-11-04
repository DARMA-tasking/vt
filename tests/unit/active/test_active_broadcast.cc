
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestActiveBroadcast : TestParameterHarnessNode {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static int handler_count;
  static int num_msg_sent;

  virtual void SetUp() {
    TestParameterHarnessNode::SetUp();

    handler_count = 0;
    num_msg_sent = 16;
  }

  virtual void TearDown() {
    TestParameterHarnessNode::TearDown();
  }

  static void test_handler(TestMsg* msg) {
    #if DEBUG_TEST_HARNESS_PRINT
      auto const& this_node = theContext()->getNode();
      fmt::print("{}: test_handler: cnt={}\n", this_node, handler_count);
    #endif

    handler_count++;
  }
};

/*static*/ int TestActiveBroadcast::handler_count;
/*static*/ int TestActiveBroadcast::num_msg_sent;

TEST_P(TestActiveBroadcast, test_type_safe_active_fn_bcast2) {
  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  NodeType const& root = GetParam();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_bcast: node={}, root={}\n", my_node, root);
  #endif

  if (root < num_nodes) {
    if (my_node == root) {
      for (int i = 0; i < num_msg_sent; i++) {
        auto msg = new TestMsg();
        theMsg()->broadcastMsg<TestMsg, test_handler>(msg, [=]{ delete msg; });
      }
    }

    theTerm()->addAction([=]{
      if (my_node != root) {
        ASSERT_TRUE(handler_count == num_msg_sent);
      }
    });
  }
}

INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestActiveBroadcast,
  ::testing::Range(static_cast<NodeType>(0), static_cast<NodeType>(16), 1)
);

}}} // end namespace vt::tests::unit
