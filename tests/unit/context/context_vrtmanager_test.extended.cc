/*
//@HEADER
// *****************************************************************************
//
//                      context_vrtmanager_test.extended.cc
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

  EXPECT_EQ(theVirtualManager()->getNode(), theContext()->getNode());
  EXPECT_EQ(theVirtualManager()->getCurrentIdent(), 0);

  auto proxy1 = theVirtualManager()->makeVirtual<HelloVirtualContext>(10);
  EXPECT_EQ(theVirtualManager()->getCurrentIdent(), 1);

  auto temp1 = theVirtualManager()->getVirtualContextByProxy(proxy1);
  auto hello1 = static_cast<HelloVirtualContext *>(temp1);
  EXPECT_EQ(hello1->from, 10);
  EXPECT_EQ(theVirtualManager()->getVirtualContextByID(1), nullptr);
  auto proxy3 = proxy1;
  VirtualContextProxy::setVirtualContextId(proxy3, 5);
  EXPECT_EQ(theVirtualManager()->getVirtualContextByProxy(proxy3), nullptr);

  auto temp2 = theVirtualManager()->getVirtualContextByProxy(proxy1);
  auto hello2 = static_cast<HelloVirtualContext *>(temp2);
  EXPECT_EQ(hello2->from, 10);

  EXPECT_EQ(VirtualContextProxy::getVirtualContextNode(proxy1),
            theVirtualManager()->getNode());
  EXPECT_EQ(VirtualContextProxy::getVirtualContextId(proxy1), 0);
  EXPECT_EQ(VirtualContextProxy::isCollection(proxy1), false);
  EXPECT_EQ(VirtualContextProxy::isMigratable(proxy1), false);

  //////////////////////////////////////////////////////////////////////////

  auto proxy2 = theVirtualManager()->makeVirtual<HelloVirtualContext>(100);
  EXPECT_EQ(theVirtualManager()->getCurrentIdent(), 2);

  auto temp3 = theVirtualManager()->getVirtualContextByProxy(proxy2);
  auto hello3 = static_cast<HelloVirtualContext *>(temp3);
  EXPECT_EQ(hello3->from, 100);

  theVirtualManager()->destroyVirtualContextByProxy(proxy1);

  EXPECT_EQ(theVirtualManager()->getVirtualContextByProxy(proxy1), nullptr);
}

}}} // end namespace vt::tests::unit
