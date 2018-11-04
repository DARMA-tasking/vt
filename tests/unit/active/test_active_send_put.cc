
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct PutTestMessage : ::vt::PayloadMessage {
  int num_ints = 0;

  explicit PutTestMessage(int const in_num_ints) : num_ints(in_num_ints) { }
  PutTestMessage() = default;
};

struct TestActiveSendPut : TestParameterHarnessNode {
  static NodeType from_node;
  static NodeType to_node;

  virtual void SetUp() {
    TestParameterHarnessNode::SetUp();
    from_node = 0;
    to_node = 1;
  }

  static void test_handler(PutTestMessage* msg) {
    auto const& this_node = theContext()->getNode();
    auto ptr = static_cast<int*>(msg->getPut());
    auto size = msg->getPutSize();
    #if DEBUG_TEST_HARNESS_PRINT
      fmt::print("{}: test_handler_2: size={}, ptr={}\n", this_node, size, ptr);
    #endif
    EXPECT_EQ(msg->num_ints * sizeof(int), size);
    for (int i = 0; i < msg->num_ints; i++) {
      EXPECT_EQ(ptr[i], i);
    }
  }
};

/*static*/ NodeType TestActiveSendPut::from_node;
/*static*/ NodeType TestActiveSendPut::to_node;

TEST_P(TestActiveSendPut, test_active_fn_send_put_param) {
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_send_large_put: node={}\n", my_node);
  #endif

  auto const& vec_size = GetParam();

  std::vector<int> test_vec_2(vec_size);
  for (int i = 0; i < vec_size; i++) {
    test_vec_2[i] = i;
  }

  if (my_node == from_node) {
    auto msg = makeSharedMessage<PutTestMessage>(
      static_cast<int>(test_vec_2.size())
    );
    msg->setPut(&test_vec_2[0], sizeof(int)*test_vec_2.size());
    #if DEBUG_TEST_HARNESS_PRINT
      fmt::print("{}: sendMsg: (put) i={}\n", my_node, i);
    #endif
    theMsg()->sendMsg<PutTestMessage, test_handler>(1, msg);
  }
}

INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestActiveSendPut,
  ::testing::Range(static_cast<NodeType>(2), static_cast<NodeType>(512), 4)
);

}}} // end namespace vt::tests::unit
