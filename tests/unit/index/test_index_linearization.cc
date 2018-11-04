
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

class TestIndex : public TestHarness {
  virtual void SetUp() {
    TestHarness::SetUp();
  }

  virtual void TearDown() {
    TestHarness::TearDown();
  }
};

TEST_F(TestIndex, test_index_1d_linearization) {
  using namespace vt;

  static constexpr int const dim1 = 92;

  index::Index1D idx(8);
  index::Index1D max_idx(dim1);

  int cur_val = 0;
  for (int i = 0; i < dim1; i++) {
    auto cur_idx = index::Index1D(i);
    auto lin_idx = mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx);
    auto lin_idx1 = mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx);

    #if DEBUG_TEST_HARNESS_PRINT
      auto cur_idx_str = cur_idx.toString().c_str();
      auto max_idx_str = max_idx.toString().c_str();
      fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
    #endif

    EXPECT_EQ(lin_idx, cur_val);
    EXPECT_EQ(lin_idx1, cur_val);

    cur_val++;
  }

  #if DEBUG_TEST_HARNESS_PRINT
    auto const& idx_str = idx.toString().c_str();
    auto const& idx_max_str = max_idx.toString().c_str();
    fmt::print("idx={}, idx_max={}\n", idx_str, idx_max_str);
  #endif

  for (int i = 92; i < 100; i++) {
    auto cur_idx = index::Index1D(i);
    ASSERT_DEATH(mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx), "Out of range index!");
    ASSERT_DEATH(mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx), "Out of range index!");
  }
}

TEST_F(TestIndex, test_index_2d_linearization) {
  using namespace vt;

  static constexpr int const dim1 = 10, dim2 = 12;

  index::Index2D idx(8, 4);
  index::Index2D max_idx(dim1, dim2);

  int cur_val = 0;
  for (int i = 0; i < dim1; i++) {
    for (int j = 0; j < dim2; j++) {
      auto cur_idx = index::Index2D(i, j);
      auto lin_idx = mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx);

      #if DEBUG_TEST_HARNESS_PRINT
        auto cur_idx_str = cur_idx.toString().c_str();
        auto max_idx_str = max_idx.toString().c_str();
        fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
      #endif

      EXPECT_EQ(lin_idx, cur_val);

      cur_val++;
    }
  }

  cur_val = 0;
  for (int j = 0; j < dim2; j++) {
    for (int i = 0; i < dim1; i++) {
      auto cur_idx = index::Index2D(i, j);
      auto lin_idx = mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx);

      #if DEBUG_TEST_HARNESS_PRINT
      auto cur_idx_str = cur_idx.toString().c_str();
        auto max_idx_str = max_idx.toString().c_str();
        fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
      #endif

      EXPECT_EQ(lin_idx, cur_val);

      cur_val++;
    }
  }

  #if DEBUG_TEST_HARNESS_PRINT
    auto const& idx_str = idx.toString().c_str();
    auto const& idx_max_str = max_idx.toString().c_str();
    fmt::print("idx={}, idx_max={}\n", idx_str, idx_max_str);
  #endif

  for (int i = 10; i < 20; i++) {
    for (int j = 12; j < 20; j++) {
      auto cur_idx = index::Index2D(i,j);
      ASSERT_DEATH(mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx), "Out of range index!");
      ASSERT_DEATH(mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx), "Out of range index!");
    }
  }
}

TEST_F(TestIndex, test_index_3d_linearization) {
  using namespace vt;

  static constexpr int const dim1 = 3, dim2 = 9, dim3 = 23;

  index::Index3D idx(1, 5, 16);
  index::Index3D max_idx(dim1, dim2, dim3);

  int cur_val = 0;
  for (int i = 0; i < dim1; i++) {
    for (int j = 0; j < dim2; j++) {
      for (int k = 0; k < dim3; k++) {
        auto cur_idx = index::Index3D(i, j, k);
        auto lin_idx = mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx);

        #if DEBUG_TEST_HARNESS_PRINT
          auto cur_idx_str = cur_idx.toString().c_str();
          auto max_idx_str = max_idx.toString().c_str();
          fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
        #endif

        EXPECT_EQ(lin_idx, cur_val);

        cur_val++;
      }
    }
  }

  cur_val = 0;
  for (int k = 0; k < dim3; k++) {
    for (int j = 0; j < dim2; j++) {
      for (int i = 0; i < dim1; i++) {
        auto cur_idx = index::Index3D(i, j, k);
        auto lin_idx = mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx);

        #if DEBUG_TEST_HARNESS_PRINT
        auto cur_idx_str = cur_idx.toString().c_str();
          auto max_idx_str = max_idx.toString().c_str();
          fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
        #endif

        EXPECT_EQ(lin_idx, cur_val);

        cur_val++;
      }
    }
  }

  #if DEBUG_TEST_HARNESS_PRINT
    auto const& idx_str = idx.toString().c_str();
    auto const& idx_max_str = max_idx.toString().c_str();
    fmt::print("idx={}, idx_max={}\n", idx_str, idx_max_str);
  #endif

  for (int i = 3; i < 10; i++) {
    for (int j = 9; j < 15; j++) {
      for (int k = 23; k < 30; k++) {
        auto cur_idx = index::Index3D(i, j, k);
        ASSERT_DEATH(mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx), "Out of range index!");
        ASSERT_DEATH(mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx), "Out of range index!");
      }
    }
  }
}

}}} // end namespace vt::tests::unit
