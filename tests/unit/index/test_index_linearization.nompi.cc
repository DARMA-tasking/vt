/*
//@HEADER
// *****************************************************************************
//
//                      test_index_linearization.nompi.cc
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

#include "vt/topos/mapping/dense/dense.h"

namespace vt { namespace tests { namespace unit { namespace linear {

struct TestIndexLinear : TestHarness { };

TEST_F(TestIndexLinear, test_index_1d_linearization) {
  using namespace vt;

  static constexpr int const dim1 = 92;

  Index1D idx(8);
  Index1D max_idx(dim1);

  int cur_val = 0;
  for (int i = 0; i < dim1; i++) {
    auto cur_idx = Index1D(i);
    auto lin_idx = mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx);
    auto lin_idx1 = mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx);

    #if DEBUG_TEST_HARNESS_PRINT
      auto cur_idx_str = cur_idx.toString().c_str();
      auto max_idx_str = max_idx.toString().c_str();
      fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
    #endif

    EXPECT_EQ(lin_idx, cur_val);
    EXPECT_EQ(lin_idx1, cur_val);

    cur_val++;
  }

  #if DEBUG_TEST_HARNESS_PRINT
    auto const& idx_str = idx.toString().c_str();
    auto const& idx_max_str = max_idx.toString().c_str();
    fmt::print("idx={}, idx_max={}\n", idx_str, idx_max_str);
  #endif

  // Tricky to assert death because the output might be multi-line and not match
  // exactly...
  #if 0
  for (int i = 92; i < 100; i++) {
    auto cur_idx = Index1D(i);
    ASSERT_DEATH(mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx), "Out of range index!");
    ASSERT_DEATH(mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx), "Out of range index!");
  }
  #endif
}

TEST_F(TestIndexLinear, test_index_2d_linearization) {
  using namespace vt;

  static constexpr int const dim1 = 10, dim2 = 12;

  Index2D idx(8, 4);
  Index2D max_idx(dim1, dim2);

  int cur_val = 0;
  for (int i = 0; i < dim1; i++) {
    for (int j = 0; j < dim2; j++) {
      auto cur_idx = Index2D(i, j);
      auto lin_idx = mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx);

      #if DEBUG_TEST_HARNESS_PRINT
        auto cur_idx_str = cur_idx.toString().c_str();
        auto max_idx_str = max_idx.toString().c_str();
        fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
      #endif

      EXPECT_EQ(lin_idx, cur_val);

      cur_val++;
    }
  }

  cur_val = 0;
  for (int j = 0; j < dim2; j++) {
    for (int i = 0; i < dim1; i++) {
      auto cur_idx = Index2D(i, j);
      auto lin_idx = mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx);

      #if DEBUG_TEST_HARNESS_PRINT
      auto cur_idx_str = cur_idx.toString().c_str();
        auto max_idx_str = max_idx.toString().c_str();
        fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
      #endif

      EXPECT_EQ(lin_idx, cur_val);

      cur_val++;
    }
  }

  #if DEBUG_TEST_HARNESS_PRINT
    auto const& idx_str = idx.toString().c_str();
    auto const& idx_max_str = max_idx.toString().c_str();
    fmt::print("idx={}, idx_max={}\n", idx_str, idx_max_str);
  #endif

  #if 0
  for (int i = 10; i < 20; i++) {
    for (int j = 12; j < 20; j++) {
      auto cur_idx = Index2D(i,j);
      ASSERT_DEATH(mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx), "Out of range index!");
      ASSERT_DEATH(mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx), "Out of range index!");
    }
  }
  #endif
}

TEST_F(TestIndexLinear, test_index_3d_linearization) {
  using namespace vt;

  static constexpr int const dim1 = 3, dim2 = 9, dim3 = 23;

  Index3D idx(1, 5, 16);
  Index3D max_idx(dim1, dim2, dim3);

  int cur_val = 0;
  for (int i = 0; i < dim1; i++) {
    for (int j = 0; j < dim2; j++) {
      for (int k = 0; k < dim3; k++) {
        auto cur_idx = Index3D(i, j, k);
        auto lin_idx = mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx);

        #if DEBUG_TEST_HARNESS_PRINT
          auto cur_idx_str = cur_idx.toString().c_str();
          auto max_idx_str = max_idx.toString().c_str();
          fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
        #endif

        EXPECT_EQ(lin_idx, cur_val);

        cur_val++;
      }
    }
  }

  cur_val = 0;
  for (int k = 0; k < dim3; k++) {
    for (int j = 0; j < dim2; j++) {
      for (int i = 0; i < dim1; i++) {
        auto cur_idx = Index3D(i, j, k);
        auto lin_idx = mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx);

        #if DEBUG_TEST_HARNESS_PRINT
        auto cur_idx_str = cur_idx.toString().c_str();
          auto max_idx_str = max_idx.toString().c_str();
          fmt::print("idx={}, max={}, lin={}\n", cur_idx_str, max_idx_str, lin_idx);
        #endif

        EXPECT_EQ(lin_idx, cur_val);

        cur_val++;
      }
    }
  }

  #if DEBUG_TEST_HARNESS_PRINT
    auto const& idx_str = idx.toString().c_str();
    auto const& idx_max_str = max_idx.toString().c_str();
    fmt::print("idx={}, idx_max={}\n", idx_str, idx_max_str);
  #endif

  #if 0
  for (int i = 3; i < 10; i++) {
    for (int j = 9; j < 15; j++) {
      for (int k = 23; k < 30; k++) {
        auto cur_idx = Index3D(i, j, k);
        ASSERT_DEATH(mapping::linearizeDenseIndexColMajor(&cur_idx, &max_idx), "Out of range index!");
        ASSERT_DEATH(mapping::linearizeDenseIndexRowMajor(&cur_idx, &max_idx), "Out of range index!");
      }
    }
  }
  #endif
}

}}}} // end namespace vt::tests::unit::linear
