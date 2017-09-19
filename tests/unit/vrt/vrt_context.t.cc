
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"

#include "vrt/vrt_context.h"

class TestVrtContext : public TestHarness {
  virtual void SetUp() {
    TestHarness::SetUp();
  }

  virtual void TearDown() {
    TestHarness::TearDown();
  }
};


TEST_F(TestVrtContext, Construction) {
  using namespace vt;

  vrt::VrtContext vrtc1(10, 20);
  EXPECT_EQ(vrtc1.isCollection(), false);
  EXPECT_EQ(vrtc1.isMigratable(), false);
  EXPECT_EQ(vrtc1.getVrtContextNode(), 10);
  EXPECT_EQ(vrtc1.getVrtContextIdentifier(), 20);

  vrt::VrtContext vrtc2(100, 200, false, true);
  EXPECT_EQ(vrtc2.isCollection(), false);
  EXPECT_EQ(vrtc2.isMigratable(), true);
  EXPECT_EQ(vrtc2.getVrtContextNode(), 100);
  EXPECT_EQ(vrtc2.getVrtContextIdentifier(), 200);
}


TEST_F(TestVrtContext, public_API) {
  using namespace vt;

  vrt::VrtContext vrtc = vrt::VrtContext();
  EXPECT_EQ(vrtc.isCollection(), false);
  EXPECT_EQ(vrtc.isMigratable(), false);

  vrtc.setVrtContextNode(100);
  EXPECT_EQ(vrtc.isCollection(), false);
  EXPECT_EQ(vrtc.isMigratable(), false);
  EXPECT_EQ(vrtc.getVrtContextNode(), 100);

  vrtc.setVrtContextIdentifier(200);
  EXPECT_EQ(vrtc.isCollection(), false);
  EXPECT_EQ(vrtc.isMigratable(), false);
  EXPECT_EQ(vrtc.getVrtContextNode(), 100);
  EXPECT_EQ(vrtc.getVrtContextIdentifier(), 200);

  vrtc.setIsCollection(true);
  EXPECT_EQ(vrtc.isCollection(), true);
  EXPECT_EQ(vrtc.isMigratable(), false);
  EXPECT_EQ(vrtc.getVrtContextNode(), 100);
  EXPECT_EQ(vrtc.getVrtContextIdentifier(), 200);

  vrtc.setIsMigratable(true);
  EXPECT_EQ(vrtc.isCollection(), true);
  EXPECT_EQ(vrtc.isMigratable(), true);
  EXPECT_EQ(vrtc.getVrtContextNode(), 100);
  EXPECT_EQ(vrtc.getVrtContextIdentifier(), 200);
}
