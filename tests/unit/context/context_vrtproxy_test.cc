/*
//@HEADER
// ************************************************************************
//
//                          context_vrtproxy_test.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

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

  VrtContext_ProxyType proxy1 = VrtContextProxy::createProxy(200, 20);
  EXPECT_EQ(VrtContextProxy::isCollection(proxy1), false);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy1), false);
  EXPECT_EQ(VrtContextProxy::getVirtualNode(proxy1), 20);
  EXPECT_EQ(VrtContextProxy::getVirtualID(proxy1), 200);

  VrtContextProxy::setIsCollection(proxy1, true);
  EXPECT_EQ(VrtContextProxy::isCollection(proxy1), true);

  VrtContextProxy::setIsMigratable(proxy1, true);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy1), true);

  VrtContextProxy::setVrtContextNode(proxy1, 2000);
  EXPECT_EQ(VrtContextProxy::getVirtualNode(proxy1), 2000);

  VrtContextProxy::setVrtContextId(proxy1, 3000);
  EXPECT_EQ(VrtContextProxy::getVirtualID(proxy1), 3000);

  EXPECT_EQ(VrtContextProxy::isCollection(proxy1), true);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy1), true);
  EXPECT_EQ(VrtContextProxy::getVirtualNode(proxy1), 2000);
  EXPECT_EQ(VrtContextProxy::getVirtualID(proxy1), 3000);

  VrtContext_ProxyType proxy2
      = VrtContextProxy::createProxy(200, 20, true);
  EXPECT_EQ(VrtContextProxy::isCollection(proxy2), true);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy2), false);
  EXPECT_EQ(VrtContextProxy::getVirtualNode(proxy2), 20);
  EXPECT_EQ(VrtContextProxy::getVirtualID(proxy2), 200);

  VrtContext_ProxyType proxy3 =
      VrtContextProxy::createProxy(200, 20, true, true);
  EXPECT_EQ(VrtContextProxy::isCollection(proxy3), true);
  EXPECT_EQ(VrtContextProxy::isMigratable(proxy3), true);
  EXPECT_EQ(VrtContextProxy::getVirtualNode(proxy3), 20);
  EXPECT_EQ(VrtContextProxy::getVirtualID(proxy3), 200);
}

}}} // end namespace vt::tests::unit
