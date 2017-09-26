
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"

#include "context/context_vrtmanager.h"
#include "context/context_vrtmessage.h"


class TestVrtContextMessage : public TestParallelHarness {
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

struct MyHelloMsg : vt::vrt::VrtContextMessage {
  int from;

  MyHelloMsg(int const& in_from)
      : vt::vrt::VrtContextMessage(), from(in_from) {}

};

void myWorkHandler (MyHelloMsg* msg, HelloVrtContext* context) {
  // do some work
}


TEST_F(TestVrtContextMessage, Construction_and_API) {
  using namespace vt;

  auto const& my_node = theContext->getNode();

  auto vrtc1 = theVrtCManager->constructVrtContext<HelloVrtContext>(10);
  MyHelloMsg* msg = new MyHelloMsg(my_node);

  theVrtCManager->sendMsg<HelloVrtContext, MyHelloMsg, myWorkHandler>
      (my_node, makeSharedMessage<TestMsg>());


  EXPECT_EQ(theVrtCManager->getCurrentIdent(), 1);

  auto temp1 = theVrtCManager->getVrtContextByID(vrtc1);
  auto hello1 = static_cast<HelloVrtContext*>(temp1);

  auto temp2 = theVrtCManager->getVrtContextByID(vrtc1);
  auto hello2 = static_cast<HelloVrtContext*>(temp2);

  EXPECT_EQ(hello1->from, 10);
  EXPECT_EQ(hello1->getVrtContextNode(), theVrtCManager->getNode());

  hello1->setIsCollection(true);
  EXPECT_EQ(hello2->isCollection(), true);

  hello2->setIsCollection(false);
  EXPECT_EQ(hello1->isCollection(), false);

  ////////////////////////////////////////////////////////////////////

  theVrtCManager->destroyVrtContextByID(vrtc1);
  EXPECT_EQ(theVrtCManager->getVrtContextByID(vrtc1), nullptr);

  auto vrtc2 = theVrtCManager->constructVrtContext<HelloVrtContext>(20);
  auto temp21 = theVrtCManager->getVrtContextByID(vrtc2);
  auto hello21 = static_cast<HelloVrtContext*>(temp21);

  auto vrtc3 = 20;
  EXPECT_EQ(theVrtCManager->getVrtContextByID(vrtc3), nullptr);

  EXPECT_EQ(hello21->from, 20);
  EXPECT_EQ(hello21->getVrtContextNode(), theVrtCManager->getNode());
}
