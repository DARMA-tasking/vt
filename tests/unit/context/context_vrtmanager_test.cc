
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"

#include "context/context_vrtmanager.h"

namespace vt { namespace tests { namespace unit {

class TestVirtualContextManager : public TestParallelHarness {
  virtual void SetUp() {
    TestParallelHarness::SetUp();
  }

  virtual void TearDown() {
    TestParallelHarness::TearDown();
  }
};

struct HelloVirtualContext : vt::vrt::VirtualContext {
  int from;

  explicit HelloVirtualContext(int const& in_from)
      : from(in_from) {}
};

TEST_F(TestVirtualContextManager, Construction_and_API) {
  using namespace vt;
  using namespace vt::vrt;

  EXPECT_EQ(theVirtualManager->getNode(), theContext->getNode());
  EXPECT_EQ(theVirtualManager->getCurrentIdent(), 0);

  auto proxy1 = theVirtualManager->makeVirtual<HelloVirtualContext>(10);
  EXPECT_EQ(theVirtualManager->getCurrentIdent(), 1);

  auto temp1 = theVirtualManager->getVirtualContextByProxy(proxy1);
  auto hello1 = static_cast<HelloVirtualContext *>(temp1);
  EXPECT_EQ(hello1->from, 10);
  EXPECT_EQ(theVirtualManager->getVirtualContextByID(1), nullptr);
  auto proxy3 = proxy1;
  VirtualContextProxy::setVirtualContextId(proxy3, 5);
  EXPECT_EQ(theVirtualManager->getVirtualContextByProxy(proxy3), nullptr);

  auto temp2 = theVirtualManager->getVirtualContextByProxy(proxy1);
  auto hello2 = static_cast<HelloVirtualContext *>(temp2);
  EXPECT_EQ(hello2->from, 10);

  EXPECT_EQ(VirtualContextProxy::getVirtualContextNode(proxy1),
            theVirtualManager->getNode());
  EXPECT_EQ(VirtualContextProxy::getVirtualContextId(proxy1), 0);
  EXPECT_EQ(VirtualContextProxy::isCollection(proxy1), false);
  EXPECT_EQ(VirtualContextProxy::isMigratable(proxy1), false);

  //////////////////////////////////////////////////////////////////////////

  auto proxy2 = theVirtualManager->makeVirtual<HelloVirtualContext>(100);
  EXPECT_EQ(theVirtualManager->getCurrentIdent(), 2);

  auto temp3 = theVirtualManager->getVirtualContextByProxy(proxy2);
  auto hello3 = static_cast<HelloVirtualContext *>(temp3);
  EXPECT_EQ(hello3->from, 100);

  theVirtualManager->destroyVirtualContextByProxy(proxy1);

  EXPECT_EQ(theVirtualManager->getVirtualContextByProxy(proxy1), nullptr);
}

}}} // end namespace vt::tests::unit
