
#include <cstring>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"
#include "test_parallel_harness.h"

#include "transport.h"

#if backend_check_enabled(openmp)
  #include <omp.h>
#else
  #include <thread>
#endif
#include <thread>

namespace vt { namespace tests { namespace unit {

using namespace vt::tests::unit;

using ::vt::util::atomic::AtomicType;

struct TestAtomic : TestHarness { };

static constexpr int const num_vals = 128;
static constexpr int const init_val = 29;

TEST_F(TestAtomic, basic_atomic_init_single_thd) {
  AtomicType<int> test1 = {init_val};
  EXPECT_EQ(test1.load(), init_val);
}

TEST_F(TestAtomic, basic_atomic_load_single_thd) {
  AtomicType<int> test1 = {init_val};
  EXPECT_EQ(test1.load(), init_val);
  auto val = test1.load();
  EXPECT_EQ(val, init_val);
}

TEST_F(TestAtomic, basic_atomic_store_single_thd) {
  AtomicType<int> test1 = {init_val};
  EXPECT_EQ(test1.load(), init_val);
  auto const store_val = init_val + 10;
  test1.store(store_val);
  auto val = test1.load();
  EXPECT_EQ(val, store_val);
}

TEST_F(TestAtomic, basic_atomic_fetch_add_single_thd) {
  AtomicType<int> test1 = {0};
  for (int i = 0; i < num_vals; i++) {
    auto val = test1.fetch_add(1);
    EXPECT_EQ(val, i);
  }
}

TEST_F(TestAtomic, basic_atomic_fetch_sub_single_thd) {
  AtomicType<int> test1 = {0};
  for (int i = 0; i < num_vals; i++) {
    auto val = test1.fetch_sub(1);
    EXPECT_EQ(val, -i);
  }
}

using ::vt::util::mutex::MutexType;

static constexpr WorkerCountType const num_workers = 8;
static MutexType test_mutex = {};
static AtomicType<int> atomic_test_val = {0};
static AtomicType<int> atomic_num = {0};
static std::vector<bool> count(num_workers);

static void testAtomicMulti() {
  auto val = atomic_test_val.fetch_add(1);
  //printf("val=%d\n", val);
  EXPECT_LE(val, num_workers);
  test_mutex.lock();
  bool const cur_count_value = count[val];
  count[val] = true;
  EXPECT_EQ(cur_count_value, false);
  test_mutex.unlock();
  if (val == num_workers - 1) {
    auto val2 = atomic_num.fetch_add(1);
    EXPECT_EQ(val2, 0);
  }
}

static AtomicType<int> atomic_test_cas = {0};
static AtomicType<int> atomic_test_slot = {0};

static void testAtomicMultiCAS() {
  int expected = atomic_test_slot.fetch_add(1);
  int desired = expected + 1;

  //printf("begin: expected=%d, desired=%d\n", expected, desired);

  bool result = false;
  do {
    result = atomic_test_cas.compare_exchange_strong(expected, desired);
  } while (!result);

  //printf("finished: expected=%d, desired=%d\n", expected, desired);
}

TEST_F(TestAtomic, basic_atomic_fetch_add_multi_thd) {
  count.resize(num_workers);

  #if backend_check_enabled(openmp)
    #pragma omp parallel num_threads(num_workers)
    testAtomicMulti();
  #elif backend_check_enabled(stdthread)
    std::vector<std::thread> thds;
    for (auto i = 0; i < num_workers; i++) {
      thds.emplace_back(std::thread(testAtomicMulti));
    }
    for (auto i = 0; i < num_workers; i++) {
      thds[i].join();
    }
  #endif

  test_mutex.lock();
  for (auto&& elm : count) {
    EXPECT_EQ(elm, true);
  }
  EXPECT_EQ(atomic_num.load(), 1);
  EXPECT_EQ(atomic_test_val.load(), num_workers);
  test_mutex.unlock();
}

TEST_F(TestAtomic, basic_atomic_cas_multi_thd) {
  count.resize(num_workers);

  #if backend_check_enabled(openmp)
    #pragma omp parallel num_threads(num_workers)
    testAtomicMultiCAS();
  #elif backend_check_enabled(stdthread)
    std::vector<std::thread> thds;
    for (auto i = 0; i < num_workers; i++) {
      thds.emplace_back(std::thread(testAtomicMultiCAS));
    }
    for (auto i = 0; i < num_workers; i++) {
      thds[i].join();
    }
  #endif

  EXPECT_EQ(atomic_test_slot.load(), num_workers);
  EXPECT_EQ(atomic_test_cas.load(), num_workers);
}

}}} // end namespace vt::tests::unit
