
#include <cstring>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"
#include "test_parallel_harness.h"

#include "vt/transport.h"

#if backend_check_enabled(openmp)
  #include <omp.h>
#else
  #include <thread>
#endif

namespace vt { namespace tests { namespace unit {

using namespace vt::tests::unit;

struct TestTLS : TestHarness { };

static constexpr int const num_vals = 128;

// Test TLS: w/o initializer, single thread, global variable
DeclareTLS(int, test_noinit_single_thd);
#define IS_TLS 0
DeclareInitVar(IS_TLS, int, test_var_noinit_single_thd, 0);

TEST_F(TestTLS, basic_tls_noinit_single_thd) {
  using namespace vt::util::tls;

  for (int i = 0; i < num_vals; i++) {
    AccessTLS(test_noinit_single_thd) = i;
    EXPECT_EQ(AccessTLS(test_noinit_single_thd), i);
  }
}

TEST_F(TestTLS, basic_var_tls_noinit_single_thd) {
  using namespace vt::util::tls;

  for (int i = 0; i < num_vals; i++) {
    AccessVar(test_var_noinit_single_thd) = i;
    EXPECT_EQ(AccessVar(test_var_noinit_single_thd), i);
  }
}

// Test TLS: w/o initializer, single thread, static global variable
DeclareStaticTLS(int, test_noinit_static_single_thd);

TEST_F(TestTLS, basic_tls_noinit_static_single_thd) {
  using namespace vt::util::tls;

  for (int i = 0; i < num_vals; i++) {
    AccessTLS(test_noinit_static_single_thd) = i;
    EXPECT_EQ(AccessTLS(test_noinit_static_single_thd), i);
  }
}

// Test TLS: w/o initializer, single thread, class static variable
struct TestClsStatic {
  DeclareClassInsideTLS(TestClsStatic, int, test_noinit_class_single_thd);
};
DeclareClassOutsideTLS(TestClsStatic, int, test_noinit_class_single_thd);

TEST_F(TestTLS, basic_tls_noinit_class_single_thd) {
  using namespace vt::util::tls;

  for (int i = 0; i < num_vals; i++) {
    AccessClassTLS(TestClsStatic, test_noinit_class_single_thd) = i;
    EXPECT_EQ(AccessClassTLS(TestClsStatic, test_noinit_class_single_thd), i);
  }
}


static constexpr int const init_val = 29;

// Test TLS: w/initializer, single thread, class static variable
struct TestClsStaticInit {
  DeclareClassInsideInitTLS(TestClsStaticInit, int, test_init_class_single_thd, init_val);
};
DeclareClassOutsideInitTLS(TestClsStaticInit, int, test_init_class_single_thd, init_val);

TEST_F(TestTLS, basic_tls_init_class_single_thd) {
  using namespace vt::util::tls;

  EXPECT_EQ(AccessClassTLS(TestClsStaticInit, test_init_class_single_thd), init_val);

  for (int i = 0; i < num_vals; i++) {
    AccessClassTLS(TestClsStaticInit, test_init_class_single_thd) = i;
    EXPECT_EQ(AccessClassTLS(TestClsStaticInit, test_init_class_single_thd), i);
  }
}

// Test TLS: w/initializer, single thread, global variable
DeclareInitTLS(int, test_init_single_thd, init_val);

TEST_F(TestTLS, basic_tls_init_single_thd) {
  using namespace vt::util::tls;

  EXPECT_EQ(AccessTLS(test_init_single_thd), init_val);

  for (int i = 0; i < num_vals; i++) {
    AccessTLS(test_init_single_thd) = i;
    EXPECT_EQ(AccessTLS(test_init_single_thd), i);
  }
}

// Test TLS: w/initializer, single thread, static global variable
DeclareStaticInitTLS(int, test_init_static_single_thd, init_val);

TEST_F(TestTLS, basic_tls_init_static_single_thd) {
  using namespace vt::util::tls;

  EXPECT_EQ(AccessTLS(test_init_static_single_thd), init_val);

  for (int i = 0; i < num_vals; i++) {
    AccessTLS(test_init_static_single_thd) = i;
    EXPECT_EQ(AccessTLS(test_init_static_single_thd), i);
  }
}

static constexpr WorkerCountType const num_workers = 8;

// Test TLS: w/o initializer, multi thread, global variable
DeclareTLS(int, test_noinit_multi_thd);

// Test TLS: w/initializer, multi thread, global variable
DeclareInitTLS(int, test_init_multi_thd, init_val);

// Test TLS: w/o initializer, multi thread, static global variable
DeclareStaticTLS(int, test_noinit_static_multi_thd);

// Test TLS: w/initializer, multi thread, static global variable
DeclareStaticInitTLS(int, test_init_static_multi_thd, init_val);

// Test TLS: w/o initializer, single thread, class static variable
struct TestClsMultiStatic {
  DeclareClassInsideTLS(TestClsMultiStatic, int, test_noinit_class_multi_thd);
};
DeclareClassOutsideTLS(TestClsMultiStatic, int, test_noinit_class_multi_thd);

// Test TLS: winitializer, single thread, class static variable
struct TestClsMultiInitStatic {
  DeclareClassInsideInitTLS(TestClsMultiInitStatic, int, test_init_class_multi_thd, init_val);
};
DeclareClassOutsideInitTLS(TestClsMultiInitStatic, int, test_init_class_multi_thd, init_val);

static void testTLSMulti(
  bool const is_static, bool const is_init, bool const is_class = false
) {
  if (is_init) {
    if (is_static) {
      EXPECT_EQ(AccessTLS(test_init_static_multi_thd), init_val);
    } else if (is_class) {
      EXPECT_EQ(AccessClassTLS(TestClsMultiInitStatic, test_init_class_multi_thd), init_val);
    } else {
      EXPECT_EQ(AccessTLS(test_init_multi_thd), init_val);
    }
  }

  if (is_init) {
    if (is_static) {
      for (int i = 0; i < num_vals; i++) {
        AccessTLS(test_init_static_multi_thd) = i;
        EXPECT_EQ(AccessTLS(test_init_static_multi_thd), i);
      }
    } else if (is_class) {
      for (int i = 0; i < num_vals; i++) {
        AccessClassTLS(TestClsMultiInitStatic, test_init_class_multi_thd) = i;
        EXPECT_EQ(AccessClassTLS(TestClsMultiInitStatic, test_init_class_multi_thd), i);
      }
    } else {
      for (int i = 0; i < num_vals; i++) {
        AccessTLS(test_init_multi_thd) = i;
        EXPECT_EQ(AccessTLS(test_init_multi_thd), i);
      }
    }
  } else {
    if (is_static) {
      for (int i = 0; i < num_vals; i++) {
        AccessTLS(test_noinit_static_multi_thd) = i;
        EXPECT_EQ(AccessTLS(test_noinit_static_multi_thd), i);
      }
    } else if (is_class) {
      for (int i = 0; i < num_vals; i++) {
        AccessClassTLS(TestClsMultiStatic, test_noinit_class_multi_thd) = i;
        EXPECT_EQ(AccessClassTLS(TestClsMultiStatic, test_noinit_class_multi_thd), i);
      }
    } else {
      for (int i = 0; i < num_vals; i++) {
        AccessTLS(test_noinit_multi_thd) = i;
        EXPECT_EQ(AccessTLS(test_noinit_multi_thd), i);
      }
    }
  }
}

#if backend_check_enabled(openmp)

TEST_F(TestTLS, basic_tls_noinit_multi_thd_openmp) {
  using namespace vt::util::tls;

  #pragma omp parallel num_threads(num_workers + 1)
  {
    testTLSMulti(false, false);
  }
}

TEST_F(TestTLS, basic_tls_init_multi_thd_openmp) {
  using namespace vt::util::tls;

  #pragma omp parallel num_threads(num_workers + 1)
  {
    WorkerIDType const thd = omp_get_thread_num();
    testTLSMulti(false, true);
  }
}

TEST_F(TestTLS, basic_tls_noinit_static_multi_thd_openmp) {
  using namespace vt::util::tls;

  #pragma omp parallel num_threads(num_workers + 1)
  {
    WorkerIDType const thd = omp_get_thread_num();
    testTLSMulti(true, false);
  }
}

TEST_F(TestTLS, basic_tls_init_static_multi_thd_openmp) {
  using namespace vt::util::tls;

  #pragma omp parallel num_threads(num_workers + 1)
  {
    WorkerIDType const thd = omp_get_thread_num();
    testTLSMulti(true, true);
  }
}

TEST_F(TestTLS, basic_tls_noinit_class_multi_thd_openmp) {
  using namespace vt::util::tls;

  #pragma omp parallel num_threads(num_workers + 1)
  {
    WorkerIDType const thd = omp_get_thread_num();
    testTLSMulti(false, false, true);
  }
}

TEST_F(TestTLS, basic_tls_init_class_multi_thd_openmp) {
  using namespace vt::util::tls;

  #pragma omp parallel num_threads(num_workers + 1)
  {
    WorkerIDType const thd = omp_get_thread_num();
    testTLSMulti(false, true, true);
  }
}

#elif backend_check_enabled(stdthread)

static void testTLSMultiNoInit() { testTLSMulti(false, false, false); }
static void testTLSMultiInit() { testTLSMulti(false, true, false); }
static void testTLSMultiStaticNoInit() { testTLSMulti(true, false, false); }
static void testTLSMultiStaticInit() { testTLSMulti(true, true, false); }
static void testTLSMultiClassNoInit() { testTLSMulti(false, false, true); }
static void testTLSMultiClassInit() { testTLSMulti(false, true, true); }

TEST_F(TestTLS, basic_tls_noinit_multi_thd_std) {
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

TEST_F(TestTLS, basic_tls_init_multi_thd_std) {
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

TEST_F(TestTLS, basic_tls_noinit_static_multi_thd_std) {
  using namespace vt::util::tls;

  std::vector<std::thread> thds;
  for (auto i = 0; i < num_workers; i++) {
    thds.emplace_back(std::thread(testTLSMultiStaticNoInit));
  }
  testTLSMultiStaticNoInit();

  for (auto i = 0; i < num_workers; i++) {
    thds[i].join();
  }
}

TEST_F(TestTLS, basic_tls_init_static_multi_thd_std) {
  using namespace vt::util::tls;

  std::vector<std::thread> thds;
  for (auto i = 0; i < num_workers; i++) {
    thds.emplace_back(std::thread(testTLSMultiStaticInit));
  }
  testTLSMultiStaticInit();

  for (auto i = 0; i < num_workers; i++) {
    thds[i].join();
  }
}

TEST_F(TestTLS, basic_tls_noinit_class_multi_thd_std) {
  using namespace vt::util::tls;

  std::vector<std::thread> thds;
  for (auto i = 0; i < num_workers; i++) {
    thds.emplace_back(std::thread(testTLSMultiClassNoInit));
  }
  testTLSMultiClassNoInit();

  for (auto i = 0; i < num_workers; i++) {
    thds[i].join();
  }
}

TEST_F(TestTLS, basic_tls_init_class_multi_thd_std) {
  using namespace vt::util::tls;

  std::vector<std::thread> thds;
  for (auto i = 0; i < num_workers; i++) {
    thds.emplace_back(std::thread(testTLSMultiClassInit));
  }
  testTLSMultiClassInit();

  for (auto i = 0; i < num_workers; i++) {
    thds[i].join();
  }
}

#endif

}}} // end namespace vt::tests::unit
