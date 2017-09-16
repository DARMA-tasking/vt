
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

using namespace vt;
using namespace vt::tests::unit;

struct TestActiveBroadcast : TestParallelHarness { };

namespace test_type_safe_active_fn_bcast_ns {

using TestMsg = TestStaticBytesShortMsg<4>;

static NodeType root_node = 0;
static int handler_count = 0;
static int num_msg_sent = 16;

static void test_handler(TestMsg* msg) {
  auto const& this_node = theContext->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    printf("%d: test_handler: cnt=%d\n", this_node, handler_count);
  #endif

  handler_count++;
}

}

TEST_F(TestActiveBroadcast, test_type_safe_active_fn_bcast) {
  using namespace test_type_safe_active_fn_bcast_ns;

  auto const& my_node = theContext->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    printf("test_type_safe_active_fn_bcast: node=%d\n", my_node);
  #endif

  if (my_node == root_node) {
    for (int i = 0; i < num_msg_sent; i++) {
      auto msg = new TestMsg();
      theMsg->broadcastMsg<TestMsg, test_handler>(msg, [=]{ delete msg; });
    }
  } else if (my_node != root_node) {
    theTerm->forceGlobalTermAction([=]{
      ASSERT_TRUE(handler_count == num_msg_sent);
      finished = true;
    });
  }
}
