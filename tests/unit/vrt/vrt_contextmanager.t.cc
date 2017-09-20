
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"

#include "vrt/vrt_contextmanager.h"


class TestVrtContextManager : public TestParallelHarness {
  virtual void SetUp() {
    TestParallelHarness::SetUp();
  }

  virtual void TearDown() {
    TestParallelHarness::TearDown();
  }
};

struct HelloVrtContext : vt::vrt::VrtContext {
  int from;

  explicit HelloVrtContext(int const& in_from)
      : VrtContext(), from(in_from) {}
};


TEST_F(TestVrtContextManager, Construction_and_API) {
  using namespace vt;

  vrt::VrtContextManager vrtM;
  EXPECT_EQ(vrtM.getNode(), theContext->getNode());
  EXPECT_EQ(vrtM.getCurrentIdent(), 0);

  auto vrtM_proxy = vrtM.newVrtContext();
  EXPECT_EQ(vrtM.getCurrentIdent(), 1);

  auto vrtH = HelloVrtContext(20);
  vrtH.setIsMigratable(true);
  vrtH.setIsCollection(true);
  EXPECT_EQ(vrtH.isCollection(), true);
  EXPECT_EQ(vrtH.isMigratable(), true);

  auto vrtH_proxy = vrtM.newVrtContext(&vrtH);
  EXPECT_EQ(vrtH.isCollection(), true);
  EXPECT_EQ(vrtH.isMigratable(), true);
  EXPECT_EQ(vrtH.getVrtContextNode(), theContext->getNode());
  EXPECT_EQ(vrtH.getVrtContextIdentifier(), 1);

  EXPECT_EQ(vrtH_proxy.isCollection(), true);
  EXPECT_EQ(vrtH_proxy.isMigratable(), true);
  EXPECT_EQ(vrtH_proxy.getVrtContextNode(), theContext->getNode());
  EXPECT_EQ(vrtH_proxy.getVrtContextIdentifier(), 1);

  EXPECT_EQ(vrtH.from, 20);
}

