
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct PutTestMessage : ::vt::PayloadMessage {
  PutTestMessage() = default;
};

struct TestActiveSend : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static NodeType from_node;
  static NodeType to_node;

  static int handler_count;
  static int num_msg_sent;

  virtual void SetUp() {
    TestParallelHarness::SetUp();

    handler_count = 0;
    num_msg_sent = 16;

    from_node = 0;
    to_node = 1;
  }

  static void test_handler_2(PutTestMessage* msg) {
    auto const& this_node = theContext()->getNode();
    auto ptr = static_cast<int*>(msg->getPut());
    auto size = msg->getPutSize();
    #if DEBUG_TEST_HARNESS_PRINT
      fmt::print("{}: test_handler_2: size={}, ptr={}\n", this_node, size, ptr);
    #endif
    EXPECT_EQ(2 * sizeof(int), size);
    for (int i = 0; i < 2; i++) {
      EXPECT_EQ(ptr[i], i);
    }
  }

  static void test_handler_3(PutTestMessage* msg) {
    auto const& this_node = theContext()->getNode();
    auto ptr = static_cast<int*>(msg->getPut());
    auto size = msg->getPutSize();
    #if DEBUG_TEST_HARNESS_PRINT
      fmt::print("{}: test_handler_3: size={}, ptr={}\n", this_node, size, ptr);
    #endif
    EXPECT_EQ(10 * sizeof(int), size);
    for (int i = 0; i < 10; i++) {
      EXPECT_EQ(ptr[i], i);
    }
  }

  static void test_handler(TestMsg* msg) {
    auto const& this_node = theContext()->getNode();

    #if DEBUG_TEST_HARNESS_PRINT
      fmt::print("{}: test_handler: cnt={}\n", this_node, handler_count);
    #endif

    handler_count++;

    EXPECT_EQ(this_node, to_node);
  }
};

/*static*/ NodeType TestActiveSend::from_node;
/*static*/ NodeType TestActiveSend::to_node;
/*static*/ int TestActiveSend::handler_count;
/*static*/ int TestActiveSend::num_msg_sent;

TEST_F(TestActiveSend, test_type_safe_active_fn_send) {
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_send: node={}\n", my_node);
  #endif

  if (my_node == from_node) {
    for (int i = 0; i < num_msg_sent; i++) {
      #if DEBUG_TEST_HARNESS_PRINT
        fmt::print("{}: sendMsg: i={}\n", my_node, i);
      #endif
      auto msg = new TestMsg();
      theMsg()->sendMsg<TestMsg, test_handler>(1, msg, [=]{ delete msg; });
    }
  } else if (my_node == to_node) {
    theTerm()->addAction([=]{
      EXPECT_EQ(handler_count, num_msg_sent);
    });
  }
}

TEST_F(TestActiveSend, test_type_safe_active_fn_send_small_put) {
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_send_small_put: node={}\n", my_node);
  #endif

  std::vector<int> test_vec{0,1};

  if (my_node == from_node) {
    for (int i = 0; i < num_msg_sent; i++) {
      auto msg = new PutTestMessage();
      msg->setPut(&test_vec[0], sizeof(int)*test_vec.size());
      #if DEBUG_TEST_HARNESS_PRINT
        fmt::print("{}: sendMsg: (put) i={}\n", my_node, i);
      #endif
      theMsg()->sendMsg<PutTestMessage, test_handler_2>(
        1, msg, [=]{ delete msg; }
      );
    }
  }
}

TEST_F(TestActiveSend, test_type_safe_active_fn_send_large_put) {
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_send_large_put: node={}\n", my_node);
  #endif

  std::vector<int> test_vec_2{0,1,2,3,4,5,6,7,8,9};

  if (my_node == from_node) {
    for (int i = 0; i < num_msg_sent; i++) {
      auto msg = new PutTestMessage();
      msg->setPut(&test_vec_2[0], sizeof(int)*test_vec_2.size());
      #if DEBUG_TEST_HARNESS_PRINT
        fmt::print("{}: sendMsg: (put) i={}\n", my_node, i);
      #endif
      theMsg()->sendMsg<PutTestMessage, test_handler_3>(
        1, msg, [=]{ delete msg; }
      );
    }
  }
}


}}} // end namespace vt::tests::unit
