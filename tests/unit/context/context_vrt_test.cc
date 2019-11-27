/*
//@HEADER
// *****************************************************************************
//
//                             context_vrt_test.cc
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

#include "vt_gtest.h"

#include "test_harness.h"

#include "vt/vrt/context/context_vrt.h"

namespace vt { namespace tests { namespace unit {

class TestVrtContext : public TestHarness {
  virtual void SetUp() {
    TestHarness::SetUp();
  }

  virtual void TearDown() {
    TestHarness::TearDown();
  }
};

//
//TEST_F(TestVrtContext, Construction) {
//  using namespace vt;
//
//  vrt::VrtContext vrtc1(10);
//  EXPECT_EQ(vrtc1.isCollection(), false);
//  EXPECT_EQ(vrtc1.isMigratable(), false);
//  EXPECT_EQ(vrtc1.getVrtContextNode(), 10);
//
//  vrt::VrtContext vrtc2(100, false, true);
//  EXPECT_EQ(vrtc2.isCollection(), false);
//  EXPECT_EQ(vrtc2.isMigratable(), true);
//  EXPECT_EQ(vrtc2.getVrtContextNode(), 100);
//}
//
//
//TEST_F(TestVrtContext, public_API) {
//  using namespace vt;
//
//  vrt::VrtContext vrtc = vrt::VrtContext();
//  EXPECT_EQ(vrtc.isCollection(), false);
//  EXPECT_EQ(vrtc.isMigratable(), false);
//
//  vrtc.setVrtContextNode(100);
//  EXPECT_EQ(vrtc.isCollection(), false);
//  EXPECT_EQ(vrtc.isMigratable(), false);
//  EXPECT_EQ(vrtc.getVrtContextNode(), 100);
//
//  vrtc.setIsCollection(true);
//  EXPECT_EQ(vrtc.isCollection(), true);
//  EXPECT_EQ(vrtc.isMigratable(), false);
//  EXPECT_EQ(vrtc.getVrtContextNode(), 100);
//
//  vrtc.setIsMigratable(true);
//  EXPECT_EQ(vrtc.isCollection(), true);
//  EXPECT_EQ(vrtc.isMigratable(), true);
//  EXPECT_EQ(vrtc.getVrtContextNode(), 100);
//}

}}} // end namespace vt::tests::unit
