
#include <cstring>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"
#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

static constexpr int64_t const min_bytes = 1;
static constexpr int64_t const max_bytes = 16384;
static constexpr int const max_test_count = 1024;
static constexpr NodeType const from_node = 0;
static constexpr NodeType const to_node = 1;

struct TestPoolMessageSizes : TestParallelHarness {
  static int count;

  virtual void SetUp() {
    TestParallelHarness::SetUp();
    count = 0;
  }

  template <int64_t num_bytes>
  using TestMsg = TestStaticBytesShortMsg<num_bytes>;

  template <int64_t num_bytes>
  static void testPoolFun(TestMsg<num_bytes>* msg);

  template <int64_t num_bytes>
  void initData(TestMsg<num_bytes>* msg, vt::tests::unit::ByteType const& val) {
    for (auto& elm : msg->payload) {
      elm = val;
    }
  }
};

/*static*/ int TestPoolMessageSizes::count;

template <int64_t num_bytes>
void TestPoolMessageSizes::testPoolFun(TestMsg<num_bytes>* prev_msg) {
  auto const& this_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("{}: test: bytes={}, cnt={}\n", this_node, num_bytes, count);
  #endif

  count++;

  NodeType const next =
    this_node == from_node ? to_node : from_node;

  if (count < max_test_count) {
    auto msg = makeSharedMessage<TestMsg<num_bytes>>();
    theMsg()->sendMsg<TestMsg<num_bytes>, testPoolFun>(
      next, msg
    );
  } else {
    auto msg = makeSharedMessage<TestMsg<num_bytes * 2>>();
    theMsg()->sendMsg<TestMsg<num_bytes * 2>, testPoolFun>(
      next, msg
    );
    count = 0;
  }
}

template <>
void TestPoolMessageSizes::testPoolFun<max_bytes>(TestMsg<max_bytes>* msg) { }

TEST_F(TestPoolMessageSizes, pool_message_sizes_alloc) {
  using namespace vt;

  auto const& my_node = theContext()->getNode();

  if (my_node == 0) {
    auto msg = makeSharedMessage<TestMsg<min_bytes>>();
    theMsg()->sendMsg<TestMsg<min_bytes>, testPoolFun>(
      to_node, msg
    );
  }
}

}}} // end namespace vt::tests::unit
