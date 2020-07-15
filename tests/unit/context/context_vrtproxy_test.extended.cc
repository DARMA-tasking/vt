/*
//@HEADER
// *****************************************************************************
//
//                      context_vrtproxy_test.extended.cc
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
