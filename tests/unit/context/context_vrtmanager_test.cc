
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"

#include "context/context_vrtmanager.h"

namespace vt { namespace tests { namespace unit {

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
      : from(in_from) {}
};

TEST_F(TestVrtContextManager, Construction_and_API) {
  using namespace vt;
  using namespace vt::vrt;

  EXPECT_EQ(theVrtCManager->getNode(), theContext->getNode());
  EXPECT_EQ(theVrtCManager->getCurrentIdent(), 0);

  auto proxy1 = theVrtCManager->constructVrtContext<HelloVrtContext>(10);
  EXPECT_EQ(theVrtCManager->getCurrentIdent(), 1);

  auto temp1 = theVrtCManager->getVrtContextByProxy(proxy1);
  auto hello1 = static_cast<HelloVrtContext *>(temp1);
  EXPECT_EQ(hello1->from, 10);
  EXPECT_EQ(theVrtCManager->getVrtContextByID(1), nullptr);
  auto proxy3 = proxy1;
  VrtContextProxy::setVrtContextId(proxy3, 5);
  EXPECT_EQ(theVrtCManager->getVrtContextByProxy(proxy3), nullptr);

  auto temp2 = theVrtCManager->getVrtContextByProxy(proxy1);
  auto hello2 = static_cast<HelloVrtContext *>(temp2);
  EXPECT_EQ(hello2->from, 10);

  EXPECT_EQ(VrtContextProxy::getVrtContextNode(proxy1),
            theVrtCManager->getNode());
  EXPECT_EQ(VrtContextProxy::getVrtContextId(proxy1), 0);
  EXPECT_EQ(VrtContextProxy::isCollection(proxy1), false);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy1), false);

  //////////////////////////////////////////////////////////////////////////

  auto proxy2 = theVrtCManager->constructVrtContext<HelloVrtContext>(100);
  EXPECT_EQ(theVrtCManager->getCurrentIdent(), 2);

  auto temp3 = theVrtCManager->getVrtContextByProxy(proxy2);
  auto hello3 = static_cast<HelloVrtContext *>(temp3);
  EXPECT_EQ(hello3->from, 100);

  theVrtCManager->destroyVrtContextByProxy(proxy1);

  EXPECT_EQ(theVrtCManager->getVrtContextByProxy(proxy1), nullptr);
}

}}} // end namespace vt::tests::unit
