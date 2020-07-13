/*
//@HEADER
// *****************************************************************************
//
//                          test_histogram_approx.cc
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

#include <vt/utils/adt/histogram_approx.h>
#include "test_harness.h"

#include <array>
#include <random>

namespace vt { namespace tests { namespace unit {

using TestHistogramApprox = TestHarness;

TEST_F(TestHistogramApprox, test_histogram_1) {
  // Basic test to ensure all values are inserted and exact when
  // unique values <  max_centroids
  vt::adt::HistogramApprox<int64_t, int64_t> h{32};

  h.add(100);
  h.add(101);
  h.add(90);
  h.add(92, 20);
  h.add(99);
  h.add(101);
  h.add(102);
  h.add(1);
  h.add(2, 3);
  h.add(2, 5);
  h.add(2);
  h.add(92, 3);

  auto data_str = h.buildContainerString();
  EXPECT_EQ(
    data_str,
    "39 1 102 [{1 1} {2 9} {90 1} {92 23} {99 1} {100 1} {101 2} {102 1}]"
  );
}

TEST_F(TestHistogramApprox, test_histogram_2) {
  // Basic test of min/max/total count/number of centroids

  constexpr int len = 10;
  std::array<int64_t, len> vals = { 3, 92, 4, 1, 24, 8, 33, 29, 55, 87 };

  vt::adt::HistogramApprox<int64_t, int64_t> h{4};

  for (int i = 0; i < len; i++) {
    h.add(vals[i]);
  }

  EXPECT_EQ(h.getMin(), 1);
  EXPECT_EQ(h.getMax(), 92);
  EXPECT_EQ(h.getCount(), len);
  EXPECT_EQ(h.getCentroids().size(), 4);
}

TEST_F(TestHistogramApprox, test_histogram_sum_3) {
  // Test that the estimation of the number of values is correct when the
  // histogram is large enough to be exact over the values inserted

  constexpr int len = 8;
  std::array<double, len> vals = { 1, 2, 3, 4, 4, 4, 5, 6 };

  vt::adt::HistogramApprox<double, int64_t> h{16};

  for (int i = 0; i < len; i++) {
    h.add(vals[i]);
  }

  fmt::print("{}\n",h.buildContainerString());

  EXPECT_EQ(h.getMin(), 1);
  EXPECT_EQ(h.getMax(), 6);
  EXPECT_EQ(h.getCount(), len);
  EXPECT_EQ(h.estimateNumValues(0.0), 0.0);
  EXPECT_EQ(h.estimateNumValues(1.0), 1.0);
  EXPECT_EQ(h.estimateNumValues(1.5), 1.0);
  EXPECT_EQ(h.estimateNumValues(2.9), 2.0);
  EXPECT_EQ(h.estimateNumValues(4.0), 6.0);
  EXPECT_EQ(h.estimateNumValues(4.5), 6.0);
  EXPECT_EQ(h.estimateNumValues(7.0), 8.0);
}

TEST_F(TestHistogramApprox, test_histogram_sum_4) {
  // Now that we know that the exact version of estimating values (sum) works
  // based on the previous test, compare exact against approximate for a normal
  // distribution with some error tolerance

  constexpr int len = 10000;

  vt::adt::HistogramApprox<double, int64_t> exact{len};
  vt::adt::HistogramApprox<double, int64_t> approx{16};

  typename std::mt19937::result_type seed = 10;
  std::mt19937 mt{seed};
  std::normal_distribution<> dist{};

  for (int i = 0; i < len; i++) {
    auto sample = dist(mt);
    exact.add(sample);
    approx.add(sample);
  }

  fmt::print("{}\n",approx.buildContainerString());

  std::array<double, 5> test_vals = {-2.0, -1.0, 0.0, 1.0, 2.0 };

  for (auto&& elm : test_vals) {
    auto a = approx.estimateNumValues(elm);
    auto e = exact.estimateNumValues(elm);
    auto rel_error = std::fabs((a - e)/e);
    fmt::print("elm={}: a={}, e={}: error={}\n", elm, a, e, rel_error);
    EXPECT_LT(rel_error, 0.2);
  }
}

TEST_F(TestHistogramApprox, test_histogram_quantile_5) {
  // Test that the quantile "estimation" is correct when the histogram is large
  // enough to compute exact quantiles

  constexpr int len = 12;
  std::array<double, len> vals = {
    0.01, 0.1, 1.0, 2.0, 3.0, 4.0, 4.0, 4.0, 4.1, 4.2, 4.3, 4.3
  };

  vt::adt::HistogramApprox<double, int64_t> h{16};

  for (int i = 0; i < len; i++) {
    h.add(vals[i]);
  }

  fmt::print("{}\n",h.buildContainerString());

  EXPECT_EQ(h.getMin(), 0.01);
  EXPECT_EQ(h.getMax(), 4.3);
  EXPECT_EQ(h.getCount(), len);
  EXPECT_EQ(h.quantile(0.0), 0.01);
  EXPECT_EQ(h.quantile(0.25), 2.0);
  EXPECT_EQ(h.quantile(0.5), 4.0);
  EXPECT_EQ(h.quantile(0.75), 4.2);
  EXPECT_EQ(h.quantile(1.0), 4.3);
}

TEST_F(TestHistogramApprox, test_histogram_quantile_6) {
  // Now that we know that the exact version of estimating quantiles works
  // based on the previous test, compare exact against approximate for a normal
  // distribution with some error tolerance

  constexpr int len = 10000;

  vt::adt::HistogramApprox<double, int64_t> exact{len};
  vt::adt::HistogramApprox<double, int64_t> approx{16};

  typename std::mt19937::result_type seed = 10;
  std::mt19937 mt{seed};
  std::normal_distribution<> dist{};

  for (int i = 0; i < len; i++) {
    auto sample = dist(mt);
    exact.add(sample);
    approx.add(sample);
  }

  fmt::print("{}\n",approx.buildContainerString());

  std::array<double, 12> test_vals = {
    0.0001, 0.001, 0.01, 0.1, 0.25, 0.35, 0.65, 0.75, 0.9, 0.99, 0.999, 0.9999
  };

  for (auto&& elm : test_vals) {
    auto a = approx.quantile(elm);
    auto e = exact.quantile(elm);
    auto rel_error = std::fabs((a - e)/e);
    fmt::print("quantile: elm={}: a={}, e={}: error={}\n", elm, a, e, rel_error);
    EXPECT_LT(rel_error, 0.2);
  }
}

TEST_F(TestHistogramApprox, test_histogram_merge_7) {
  // Test that merging two histogram results in an optimal histogram by merging
  // the closest centroids across both histograms

  vt::adt::HistogramApprox<double, int64_t> h1{8};

  h1.add(1.);
  h1.add(2.);
  h1.add(3.);
  h1.add(4.);
  h1.add(5.);
  h1.add(6.);
  h1.add(7.);

  auto h1p = h1;

  vt::adt::HistogramApprox<double, int64_t> h2{8};

  std::array<double, 3> arr = { 7.6, 8.2, 7.7 };

  for (auto&& elm : arr) {
    h2.add(elm);
  }

  h1.mergeIn(h2);

  auto centroids = h1.getCentroids();

  // Must be 8 centroids if merge/max was applied correctly
  EXPECT_EQ(centroids.size(), 8);

  // We should have all the original centroids, except the last one, which
  // should be a single combined centroids of the last three values if optimal
  // centroids were created
  for (int i = 0; i < 7; i++) {
    EXPECT_EQ(centroids[i].getCount(), 1);
  }

  EXPECT_EQ(centroids[7].getCount(), 3);
}


}}} /* end namespace vt::tests::unit */
