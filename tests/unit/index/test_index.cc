
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"

#include "transport.h"

class TestIndex : public TestHarness {
  virtual void SetUp() {
    TestHarness::SetUp();
  }

  virtual void TearDown() {
    TestHarness::TearDown();
  }
};

TEST_F(TestIndex, test_index_1d) {
  using namespace vt;

  static constexpr index::Index1D::DenseIndexType const val = 29;

  index::Index1D idx(val);

  ASSERT_TRUE(idx[0] == val);
  ASSERT_TRUE(idx.x() == val);
  ASSERT_TRUE(idx.getSize() == val);
  ASSERT_TRUE(idx.get(0) == val);
}

TEST_F(TestIndex, test_index_2d) {
  using namespace vt;

  static constexpr index::Index2D::DenseIndexType const val1 = 29;
  static constexpr index::Index2D::DenseIndexType const val2 = 34;

  index::Index2D idx(val1, val2);

  ASSERT_TRUE(idx[0] == val1 and idx[1] == val2);
  ASSERT_TRUE(idx.x() == val1 and idx.y() == val2);
  ASSERT_TRUE(idx.get(0) == val1 and idx.get(1) == val2);
  ASSERT_TRUE(idx.getSize() == val1 * val2);
}

TEST_F(TestIndex, test_index_3d) {
  using namespace vt;

  static constexpr index::Index3D::DenseIndexType const val1 = 29;
  static constexpr index::Index3D::DenseIndexType const val2 = 34;
  static constexpr index::Index3D::DenseIndexType const val3 = 92;

  index::Index3D idx(val1, val2, val3);

  ASSERT_TRUE(idx[0] == val1 and idx[1] == val2 and idx[2] == val3);
  ASSERT_TRUE(idx.x() == val1 and idx.y() == val2 and idx.z() == val3);
  ASSERT_TRUE(idx.get(0) == val1 and idx.get(1) == val2 and idx.get(2) == val3);
  ASSERT_TRUE(idx.getSize() == val1 * val2 * val3);
}
