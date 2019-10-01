/*
//@HEADER
// *****************************************************************************
//
//                          context_vrtmessage_test.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/vrt/context/context_vrtmessage.h"

namespace vt { namespace tests { namespace unit {

class TestVirtualContextMessage : public TestParallelHarness {
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
      : VirtualContext(), from(in_from) {}
};

struct MyHelloMsg : vt::vrt::VirtualContextMessage {
  int from;

  MyHelloMsg(int const& in_from)
      : vt::vrt::VirtualContextMessage(), from(in_from) {}

};

void myWorkHandler (MyHelloMsg* msg, HelloVirtualContext* context) {
  // do some work
}


TEST_F(TestVirtualContextMessage, Construction_and_API) {
  using namespace vt;

  auto const& my_node = theContext()->getNode();

  auto vrtc1 = theVirtualManager()->makeVirtual<HelloVirtualContext>(10);
  MyHelloMsg* msg = new MyHelloMsg(my_node);

  theVirtualManager()->sendMsg<HelloVirtualContext, MyHelloMsg, myWorkHandler>
    (my_node, makeSharedMessage<MyHelloMsg>());


  EXPECT_EQ(theVirtualManager()->getCurrentIdent(), 1);

  auto temp1 = theVirtualManager()->getVirtualByID(vrtc1);
  auto hello1 = static_cast<HelloVirtualContext*>(temp1);

  auto temp2 = theVirtualManager()->getVirtualByID(vrtc1);
  auto hello2 = static_cast<HelloVirtualContext*>(temp2);

  EXPECT_EQ(hello1->from, 10);
  EXPECT_EQ(hello1->theVirtualContextNode(), getVirtualManager()->getNode());

  hello1->setIsCollection(true);
  EXPECT_EQ(hello2->isCollection(), true);

  hello2->setIsCollection(false);
  EXPECT_EQ(hello1->isCollection(), false);

  ////////////////////////////////////////////////////////////////////

  theVirtualManager()->destroyVirtualContextByID(vrtc1);
  EXPECT_EQ(theVirtualManager()->getVirtualByID(vrtc1), nullptr);

  auto vrtc2 = theVirtualManager()->makeVirtual<HelloVirtualContext>(20);
  auto temp21 = theVirtualManager()->getVirtualByID(vrtc2);
  auto hello21 = static_cast<HelloVirtualContext*>(temp21);

  auto vrtc3 = 20;
  EXPECT_EQ(theVirtualManager()->getVirtualByID(vrtc3), nullptr);

  EXPECT_EQ(hello21->from, 20);
  EXPECT_EQ(hello21->theVirtualContextNode(), getVirtualManager()->getNode());
}

}}} // end namespace vt::tests::unit
