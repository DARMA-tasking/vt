
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct PutTestMessage : ::vt::PayloadMessage {
  PutTestMessage() = default;
};

struct TestActiveBroadcastPut : TestParameterHarnessNode {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static int handler_count;
  static int num_msg_sent;
  static size_t put_size;

  virtual void SetUp() {
    TestParameterHarnessNode::SetUp();
    handler_count = 0;
    num_msg_sent = 1;
  }

  virtual void TearDown() {
    TestParameterHarnessNode::TearDown();
  }

  static void test_handler(PutTestMessage* msg) {
    #if DEBUG_TEST_HARNESS_PRINT || 1
      auto const& this_node = theContext()->getNode();
      fmt::print("{}: test_handler: cnt={}\n", this_node, handler_count);
    #endif

    auto const ptr = static_cast<int*>(msg->getPut());
    auto const size = msg->getPutSize();

    #if DEBUG_TEST_HARNESS_PRINT || 1
      fmt::print(
        "{}: test_handler: size={}, ptr={}\n", this_node, size, print_ptr(ptr)
      );
    #endif

    EXPECT_EQ(put_size * sizeof(int), size);

    for (int i = 0; i < put_size; i++) {
      EXPECT_EQ(ptr[i], i + 1);
    }

    handler_count++;
  }
};

/*static*/ int TestActiveBroadcastPut::handler_count;
/*static*/ int TestActiveBroadcastPut::num_msg_sent;
/*static*/ size_t TestActiveBroadcastPut::put_size = 4;

TEST_P(TestActiveBroadcastPut, test_type_safe_active_fn_bcast2) {
  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  NodeType const& root = GetParam();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_bcast: node={}, root={}\n", my_node, root);
  #endif

  std::vector<int> put_payload;

  for (int i = 0; i < put_size; i++) {
    put_payload.push_back(i + 1);
  }

  if (root < num_nodes) {
    if (my_node == root) {
      for (int i = 0; i < num_msg_sent; i++) {
        auto msg = new PutTestMessage();
        msg->setPut(&put_payload[0], put_size * sizeof(int));
        theMsg()->broadcastMsg<PutTestMessage, test_handler>(
          msg, [=]{ delete msg; }
        );
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
  InstantiationName, TestActiveBroadcastPut,
  ::testing::Range(static_cast<NodeType>(0), static_cast<NodeType>(16), 1)
);

}}} // end namespace vt::tests::unit
