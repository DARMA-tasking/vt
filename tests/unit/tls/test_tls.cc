
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

namespace vt { namespace tests { namespace unit {

using namespace vt::tests::unit;

struct TestTLS : TestHarness { };

static constexpr int const num_vals = 128;

DeclareTLS(int, test_noinit_single_thd);

TEST_F(TestTLS, basic_tls_noinit_single_thd) {
  using namespace vt::util::tls;

  for (int i = 0; i < num_vals; i++) {
    AccessTLS(test_noinit_single_thd) = i;
    EXPECT_EQ(AccessTLS(test_noinit_single_thd), i);
  }
}

static constexpr int const init_val = 29;

DeclareInitTLS(int, test_init_single_thd, init_val);

TEST_F(TestTLS, basic_tls_init_single_thd) {
  using namespace vt::util::tls;

  EXPECT_EQ(AccessTLS(test_init_single_thd), init_val);

  for (int i = 0; i < num_vals; i++) {
    AccessTLS(test_init_single_thd) = i;
    EXPECT_EQ(AccessTLS(test_init_single_thd), i);
  }
}

static constexpr WorkerCountType const num_workers = 8;

DeclareTLS(int, test_noinit_multi_thd);
DeclareInitTLS(int, test_init_multi_thd, init_val);

#if backend_check_enabled(openmp)

static void testTLSMulti(bool const is_init, worker::WorkerIDType const thd) {
  //printf("thd=%d\n", thd);

  if (is_init) {
    EXPECT_EQ(AccessTLS(test_init_multi_thd), init_val);
  }

  if (is_init) {
    for (int i = 0; i < num_vals; i++) {
      AccessTLS(test_init_multi_thd) = i;
      EXPECT_EQ(AccessTLS(test_init_multi_thd), i);
    }
  } else {
    for (int i = 0; i < num_vals; i++) {
      AccessTLS(test_noinit_multi_thd) = i;
      EXPECT_EQ(AccessTLS(test_noinit_multi_thd), i);
    }
  }
}

TEST_F(TestTLS, basic_tls_noinit_multi_thd) {
  using namespace vt::util::tls;

  #pragma omp parallel num_threads(num_workers + 1)
  {
    worker::WorkerIDType const thd = omp_get_thread_num();
    testTLSMulti(false, thd);
  }
}

TEST_F(TestTLS, basic_tls_init_multi_thd) {
  using namespace vt::util::tls;

  #pragma omp parallel num_threads(num_workers + 1)
  {
    worker::WorkerIDType const thd = omp_get_thread_num();
    testTLSMulti(true, thd);
  }
}

#else

static void testTLSMultiNoInit() {
  for (int i = 0; i < num_vals; i++) {
    AccessTLS(test_noinit_multi_thd) = i;
    EXPECT_EQ(AccessTLS(test_noinit_multi_thd), i);
  }
}

static void testTLSMultiInit() {
  EXPECT_EQ(AccessTLS(test_init_multi_thd), init_val);

  for (int i = 0; i < num_vals; i++) {
    AccessTLS(test_init_multi_thd) = i;
    EXPECT_EQ(AccessTLS(test_init_multi_thd), i);
  }
}

TEST_F(TestTLS, basic_tls_noinit_multi_thd) {
  using namespace vt::util::tls;

  std::vector<std::thread> thds;
  for (auto i = 0; i < num_workers; i++) {
    thds.emplace_back(std::thread(testTLSMultiNoInit));
  }

  testTLSMultiNoInit();

  for (auto i = 0; i < num_workers; i++) {
    thds[i].join();
  }
}

TEST_F(TestTLS, basic_tls_init_multi_thd) {
  using namespace vt::util::tls;

  std::vector<std::thread> thds;
  for (auto i = 0; i < num_workers; i++) {
    thds.emplace_back(std::thread(testTLSMultiInit));
  }

  testTLSMultiInit();

  for (auto i = 0; i < num_workers; i++) {
    thds[i].join();
  }
}


#endif

}}} // end namespace vt::tests::unit
