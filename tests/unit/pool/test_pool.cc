
#include <cstring>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"
#include "test_parallel_harness.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt::tests::unit;

static constexpr int64_t const min_bytes = 1;
static constexpr int64_t const max_bytes = 16384;

struct TestPool : TestParallelHarness {
  template <int64_t num_bytes>
  using TestMsg = TestStaticBytesShortMsg<num_bytes>;

  template <int64_t num_bytes>
  static void testPoolFun(TestMsg<num_bytes>* prev_msg);

  virtual void SetUp() {
    TestParallelHarness::SetUp();
  }
};

template <int64_t num_bytes>
void TestPool::testPoolFun(TestMsg<num_bytes>* prev_msg) {
  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("testPoolFun: num_bytes={}\n", num_bytes);
  #endif

  auto msg = new TestMsg<num_bytes * 2>();
  testPoolFun<num_bytes * 2>(msg);
  delete msg;
}

template <>
void TestPool::testPoolFun<max_bytes>(TestMsg<max_bytes>* msg) { }

TEST_F(TestPool, pool_message_alloc) {
  using namespace vt;

  auto const& my_node = theContext()->getNode();

  if (my_node == 0) {
    auto msg = new TestMsg<min_bytes>();
    testPoolFun<min_bytes>(msg);
    delete msg;
  }
}


TEST_F(TestPool, pool_alloc) {
  using namespace vt;

  using CharType = unsigned char;

  static constexpr size_t const max_bytes = 16384;
  static constexpr CharType const init_val = 'z';

  std::unique_ptr<pool::Pool> testPool = std::make_unique<pool::Pool>();

  for (size_t cur_bytes = 1; cur_bytes < max_bytes; cur_bytes *= 2) {
    void* ptr = testPool->alloc(cur_bytes);
    std::memset(ptr, init_val, cur_bytes);
    //fmt::print("alloc {} bytes, ptr={}\n", cur_bytes, ptr);
    EXPECT_NE(ptr, nullptr);
    for (size_t i = 0; i < cur_bytes; i++) {
      EXPECT_EQ(static_cast<CharType*>(ptr)[i], init_val);
    }
    testPool->dealloc(ptr);
  }
}

}}} // end namespace vt::tests::unit
