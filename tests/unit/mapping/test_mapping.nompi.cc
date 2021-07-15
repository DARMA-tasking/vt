/*
//@HEADER
// *****************************************************************************
//
//                            test_mapping.nompi.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/topos/mapping/mapping.h"
#include "vt/topos/mapping/dense/dense.h"

#include <vector>

namespace vt { namespace tests { namespace unit {

struct TestMapping : TestHarness { };

TEST_F(TestMapping, test_mapping_block_1d) {
  using namespace vt;

  static constexpr Index1D::DenseIndexType const val = 16;
  static constexpr Index1D::DenseIndexType const max = 256;
  static constexpr NodeType const nnodes = 8;

  Index1D idx(val);
  Index1D max_idx(max);

  EXPECT_EQ(idx[0], val);
  EXPECT_EQ(max_idx[0], max);

  std::vector<Index1D::IndexSizeType> map_cnt(nnodes);

  EXPECT_EQ(max % nnodes, 0);

  for (int i = 0; i < max; i++) {
    Index1D idx_test(i);
    auto const& node = mapping::dense1DBlockMap(&idx_test, &max_idx, nnodes);
    //fmt::print("node={},idx val={}, idx max={}\n", node, idx_test[0], max_idx[0]);
    map_cnt[node]++;
  }

  EXPECT_EQ(map_cnt.size(), static_cast<size_t>(nnodes));

  Index1D::IndexSizeType map_count_fst = map_cnt[0];

  for (int i = 1; i < nnodes; i++) {
    EXPECT_EQ(map_cnt[i], map_count_fst);
  }
}

TEST_F(TestMapping, test_mapping_block_2d) {
  using namespace vt;

  static constexpr NodeType const nnodes = 8;

  using IndexType = Index2D::DenseIndexType;

  std::array<std::array<IndexType, 2>, 5> sizes{{
    {{16,16}}, {{4,64}}, {{64,4}}, {{8,32}}, {{32,8}}
  }};

  for (auto&& elm : sizes) {
    Index2D::DenseIndexType max0 = std::get<0>(elm);
    Index2D::DenseIndexType max1 = std::get<1>(elm);

    Index2D max_idx(max0, max1);

    EXPECT_EQ(max_idx[0], max0);
    EXPECT_EQ(max_idx[1], max1);

    std::vector<Index2D::IndexSizeType> map_cnt(nnodes);

    EXPECT_EQ((max0 * max1) % nnodes, 0);

    for (int i = 0; i < max_idx[0]; i++) {
      for (int j = 0; j < max_idx[1]; j++) {
        Index2D idx_test(i, j);
        auto const& node = mapping::dense2DBlockMap(&idx_test, &max_idx, nnodes);
        // fmt::print(
        //   "node={},idx val=[{},{}], idx max=[{},{}]\n",
        //   node, idx_test[0], idx_test[1], max_idx[0], max_idx[1]
        // );
        map_cnt[node]++;
      }
    }

    EXPECT_EQ(map_cnt.size(), static_cast<size_t>(nnodes));

    Index2D::IndexSizeType map_count_fst = map_cnt[0];

    for (int i = 1; i < nnodes; i++) {
      EXPECT_EQ(map_cnt[i], map_count_fst);
    }
  }
}

TEST_F(TestMapping, test_mapping_block_3d) {
  using namespace vt;

  static constexpr NodeType const nnodes = 8;

  using IndexType = Index2D::DenseIndexType;

  std::array<std::array<IndexType, 3>, 14> sizes{{
    {{4,8,8}}, {{8,8,4}}, {{8,4,8}}, {{2,16,8}}, {{8,16*2}}, {{1,32,8}},
    {{32,1,8}}, {{32,8,1}}, {{1,64,4}}, {{64,1,4}}, {{4,64,1}}, {{1,1,256}},
    {{256,1,1}}, {{1,256,1}}
  }};

  for (auto&& elm : sizes) {
    Index3D::DenseIndexType max0 = std::get<0>(elm);
    Index3D::DenseIndexType max1 = std::get<1>(elm);
    Index3D::DenseIndexType max2 = std::get<2>(elm);

    Index3D max_idx(max0, max1, max2);

    EXPECT_EQ(max_idx[0], max0);
    EXPECT_EQ(max_idx[1], max1);
    EXPECT_EQ(max_idx[2], max2);

    std::vector<Index3D::IndexSizeType> map_cnt(nnodes);

    EXPECT_EQ((max0 * max1 * max2) % nnodes, 0);

    for (int i = 0; i < max_idx[0]; i++) {
      for (int j = 0; j < max_idx[1]; j++) {
        for (int k = 0; k < max_idx[2]; k++) {
          Index3D idx_test(i, j, k);
          auto const& node = mapping::dense3DBlockMap(&idx_test, &max_idx, nnodes);
          map_cnt[node]++;
        }
      }
    }

    EXPECT_EQ(map_cnt.size(), static_cast<size_t>(nnodes));

    Index3D::IndexSizeType map_count_fst = map_cnt[0];

    for (int i = 1; i < nnodes; i++) {
      EXPECT_EQ(map_cnt[i], map_count_fst);
    }
  }
}

}}} // end namespace vt::tests::unit
