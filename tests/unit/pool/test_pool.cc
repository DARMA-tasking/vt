
#include <cstring>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"
#include "test_parallel_harness.h"

#include "transport.h"

struct TestPoolSimple : TestHarness { };
struct TestPool : TestParallelHarness { };

TEST_F(TestPoolSimple, pool_alloc) {
  using namespace vt;

  using CharType = unsigned char;

  static constexpr size_t const max_bytes = 16384;
  static constexpr CharType const init_val = 'z';

  std::unique_ptr<pool::Pool> testPool = std::make_unique<pool::Pool>();

  for (size_t cur_bytes = 1; cur_bytes < max_bytes; cur_bytes *= 2) {
    void* ptr = testPool->alloc(cur_bytes);
    std::memset(ptr, init_val, cur_bytes);
    //printf("alloc %ld bytes, ptr=%p\n", cur_bytes, ptr);
    ASSERT_TRUE(ptr != nullptr);
    for (size_t i = 0; i < cur_bytes; i++) {
      ASSERT_TRUE(static_cast<CharType*>(ptr)[i] == init_val);
    }
    testPool->dealloc(ptr);
  }
}

namespace pool_message_alloc_ns {

static constexpr int64_t const min_bytes = 1;
static constexpr int64_t const max_bytes = 16384;

template <int64_t num_bytes>
struct TestMsg : vt::ShortMessage {
  std::array<char, num_bytes> _;
  TestMsg() : vt::ShortMessage() { }
};

template <int64_t num_bytes>
static void testPoolFun(TestMsg<num_bytes>* prev_msg) {
  printf("testPoolFun: num_bytes=%lld\n", num_bytes);

  auto msg = new TestMsg<num_bytes * 2>();
  testPoolFun<num_bytes * 2>(msg);
  delete msg;
}

template <>
void testPoolFun<max_bytes>(TestMsg<max_bytes>* msg) {
}

} // end namespace pool_message_alloc_ns

TEST_F(TestPool, pool_message_alloc) {
  using namespace vt;
  using namespace pool_message_alloc_ns;

  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    auto msg = new TestMsg<min_bytes>();
    testPoolFun<min_bytes>(msg);
    delete msg;
  }

  theTerm->forceGlobalTermAction([=]{
    finished = true;
  });
}
