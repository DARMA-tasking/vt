
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestTermReset : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static int32_t handler_count;

  static void test_handler(TestMsg* msg) {
    handler_count++;
  }
};

/*static*/ int32_t TestTermReset::handler_count = 0;

TEST_F(TestTermReset, test_termination_reset_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNode();

  if (num_nodes < 2) {
    return;
  }

  if (this_node == 0) {
    auto msg = makeSharedMessage<TestMsg>();
    theMsg()->sendMsg<TestMsg, test_handler>(1, msg);
  } else if (this_node == 1) {
    theTerm()->addAction([=]{
      EXPECT_EQ(handler_count, 1);
    });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  rt->reset();

  if (this_node == 1) {
    auto msg = makeSharedMessage<TestMsg>();
    theMsg()->sendMsg<TestMsg, test_handler>(0, msg);
  } else if (this_node == 0) {
    theTerm()->addAction([=]{
      EXPECT_EQ(handler_count, 1);
    });
  }
}

}}} // end namespace vt::tests::unit
