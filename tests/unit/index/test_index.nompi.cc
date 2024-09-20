/*
//@HEADER
// *****************************************************************************
//
//                             test_index.nompi.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#include "vt/topos/index/index.h"
#include "vt/topos/index/dense/dense_array.h"

namespace vt { namespace tests { namespace unit {

class TestIndex : public TestHarness {
  virtual void SetUp() override {
    TestHarness::SetUp();
  }

  virtual void TearDown() override {
    TestHarness::TearDown();
  }
};

TEST_F(TestIndex, test_index_1d) {
  using namespace vt;

  static constexpr Index1D::DenseIndexType const val = 29;

  Index1D idx(val);

  EXPECT_EQ(idx[0], val);
  EXPECT_EQ(idx.x(), val);
  EXPECT_EQ(idx.getSize(), static_cast<decltype(idx.getSize())>(val));
  EXPECT_EQ(idx.get(0), val);
}

TEST_F(TestIndex, test_index_2d) {
  using namespace vt;

  static constexpr Index2D::DenseIndexType const val1 = 29;
  static constexpr Index2D::DenseIndexType const val2 = 34;

  Index2D idx(val1, val2);

  EXPECT_EQ(idx[0], val1);
  EXPECT_EQ(idx.x(), val1);
  EXPECT_EQ(idx.get(0), val1);

  EXPECT_EQ(idx[1], val2);
  EXPECT_EQ(idx.y(), val2);
  EXPECT_EQ(idx.get(1), val2);

  EXPECT_EQ(idx.getSize(), static_cast<decltype(idx.getSize())>(val1 * val2));
}

TEST_F(TestIndex, test_index_3d) {
  using namespace vt;

  static constexpr Index3D::DenseIndexType const val1 = 29;
  static constexpr Index3D::DenseIndexType const val2 = 34;
  static constexpr Index3D::DenseIndexType const val3 = 92;

  Index3D idx(val1, val2, val3);

  EXPECT_EQ(idx[0], val1);
  EXPECT_EQ(idx.x(), val1);
  EXPECT_EQ(idx.get(0), val1);

  EXPECT_EQ(idx[1], val2);
  EXPECT_EQ(idx.y(), val2);
  EXPECT_EQ(idx.get(1), val2);

  EXPECT_EQ(idx[2], val3);
  EXPECT_EQ(idx.z(), val3);
  EXPECT_EQ(idx.get(2), val3);

  EXPECT_EQ(idx.getSize(), static_cast<decltype(idx.getSize())>(val1 * val2 * val3));
}

}}} // end namespace vt::tests::unit
