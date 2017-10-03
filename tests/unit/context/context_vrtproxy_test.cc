
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"

namespace vt { namespace tests { namespace unit {

class TestVrtContextProxy : public TestHarness {
  virtual void SetUp() {
    TestHarness::SetUp();
  }

  virtual void TearDown() {
    TestHarness::TearDown();
  }
};

TEST_F(TestVrtContextProxy, Construction_AND_API) {
  using namespace vt;
  using namespace vt::vrt;

  VrtContext_ProxyType proxy1 = VrtContextProxy::createNewProxy(200, 20);
  EXPECT_EQ(VrtContextProxy::isCollection(proxy1), false);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy1), false);
  EXPECT_EQ(VrtContextProxy::getVrtContextNode(proxy1), 20);
  EXPECT_EQ(VrtContextProxy::getVrtContextId(proxy1), 200);

  VrtContextProxy::setIsCollection(proxy1, true);
  EXPECT_EQ(VrtContextProxy::isCollection(proxy1), true);

  VrtContextProxy::setIsMigratable(proxy1, true);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy1), true);

  VrtContextProxy::setVrtContextNode(proxy1, 2000);
  EXPECT_EQ(VrtContextProxy::getVrtContextNode(proxy1), 2000);

  VrtContextProxy::setVrtContextId(proxy1, 3000);
  EXPECT_EQ(VrtContextProxy::getVrtContextId(proxy1), 3000);

  EXPECT_EQ(VrtContextProxy::isCollection(proxy1), true);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy1), true);
  EXPECT_EQ(VrtContextProxy::getVrtContextNode(proxy1), 2000);
  EXPECT_EQ(VrtContextProxy::getVrtContextId(proxy1), 3000);

  VrtContext_ProxyType proxy2
      = VrtContextProxy::createNewProxy(200, 20, true);
  EXPECT_EQ(VrtContextProxy::isCollection(proxy2), true);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy2), false);
  EXPECT_EQ(VrtContextProxy::getVrtContextNode(proxy2), 20);
  EXPECT_EQ(VrtContextProxy::getVrtContextId(proxy2), 200);

  VrtContext_ProxyType proxy3 =
      VrtContextProxy::createNewProxy(200, 20, true, true);
  EXPECT_EQ(VrtContextProxy::isCollection(proxy3), true);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy3), true);
  EXPECT_EQ(VrtContextProxy::getVrtContextNode(proxy3), 20);
  EXPECT_EQ(VrtContextProxy::getVrtContextId(proxy3), 200);
}

}}} // end namespace vt::tests::unit
