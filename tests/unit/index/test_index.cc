
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

TEST_F(TestIndex, test_index_1d) {
  using namespace vt;

  static constexpr Index1D::DenseIndexType const val = 29;

  Index1D idx(val);

  EXPECT_EQ(idx[0], val);
  EXPECT_EQ(idx.x(), val);
  EXPECT_EQ(idx.getSize(), val);
  EXPECT_EQ(idx.get(0), val);
}

TEST_F(TestIndex, test_index_2d) {
  using namespace vt;

  static constexpr Index2D::DenseIndexType const val1 = 29;
  static constexpr Index2D::DenseIndexType const val2 = 34;

  Index2D idx(val1, val2);

  EXPECT_EQ(idx[0], val1);
  EXPECT_EQ(idx.x(), val1);
  EXPECT_EQ(idx.get(0), val1);

  EXPECT_EQ(idx[1], val2);
  EXPECT_EQ(idx.y(), val2);
  EXPECT_EQ(idx.get(1), val2);

  EXPECT_EQ(idx.getSize(), val1 * val2);
}

TEST_F(TestIndex, test_index_3d) {
  using namespace vt;

  static constexpr Index3D::DenseIndexType const val1 = 29;
  static constexpr Index3D::DenseIndexType const val2 = 34;
  static constexpr Index3D::DenseIndexType const val3 = 92;

  Index3D idx(val1, val2, val3);

  EXPECT_EQ(idx[0], val1);
  EXPECT_EQ(idx.x(), val1);
  EXPECT_EQ(idx.get(0), val1);

  EXPECT_EQ(idx[1], val2);
  EXPECT_EQ(idx.y(), val2);
  EXPECT_EQ(idx.get(1), val2);

  EXPECT_EQ(idx[2], val3);
  EXPECT_EQ(idx.z(), val3);
  EXPECT_EQ(idx.get(2), val3);

  EXPECT_EQ(idx.getSize(), val1 * val2 * val3);
}

}}} // end namespace vt::tests::unit
