/*
//@HEADER
// ************************************************************************
//
//                      test_integral_set.cc
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

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"
#include "vt/termination/interval/integral_set.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestIntegralSet : TestParallelHarness { };

TEST_F(TestIntegralSet, test_interval_set) {
  std::vector<int> elms{6,9,2,13,8,14,10,7,5};
  std::vector<int> comp{1,2,3,4 ,4,4 ,4 ,3,3};
  vt::IntegralSet<int> i = {};
  std::set<int> t = {};
  int cur = 0;
  for (auto&& e : elms) {
    i.insert(e);
    t.insert(e);
    EXPECT_EQ(i.compressedSize(), comp[cur]);
    cur++;
    fmt::print(
      "size={}, compressed size={}, ratio={}\n",
      i.size(), i.compressedSize(), i.compression()
    );
    //i.dumpState();
  }

  EXPECT_EQ(i.size(), 9);

  for (auto&& e : elms) {
    EXPECT_EQ(i.exists(e), true);
  }

  for (int k = 0; k < 20; k++) {
    auto in_t = t.find(k) != t.end();
    EXPECT_EQ(i.exists(k), in_t);
  }

  auto ti = t.begin();
  for (auto&& e : i) {
    EXPECT_EQ(e, *ti);
    ++ti;
  }
  auto tr = t.rbegin();
  for (auto iter = i.rbegin(); iter != i.rend(); ++iter) {
    auto const& e = *iter;
    EXPECT_EQ(e, *tr);
    ++tr;
  }

  auto in = i.ibegin();
  EXPECT_EQ(*in, vt::Interval<int>(2,2));
  ++in;
  EXPECT_EQ(*in, vt::Interval<int>(5,10));
  ++in;
  EXPECT_EQ(*in, vt::Interval<int>(13,14));
}

TEST_F(TestIntegralSet, test_interval_set_2) {
  vt::IntegralSet<int> i = {};

  int num = 0;
  for (int k = 0; k < 10000; k += 2) {
    i.insert(k);
    EXPECT_TRUE(i.exists(k));
    num++;
  }

  EXPECT_EQ(num, i.size());
  EXPECT_EQ(num, i.compressedSize());

  fmt::print(
    "size={}, compressed size={}, ratio={}\n",
    i.size(), i.compressedSize(), i.compression()
  );

  for (int k = 1; k < 10000; k += 2) {
    i.insert(k);
    EXPECT_TRUE(i.exists(k));
    num++;
  }

  EXPECT_EQ(num, i.size());
  EXPECT_EQ(1, i.compressedSize());

  fmt::print(
    "size={}, compressed size={}, ratio={}\n",
    i.size(), i.compressedSize(), i.compression()
  );

  for (int k = 1; k < 10000; k += 2) {
    i.erase(k);
    EXPECT_FALSE(i.exists(k));
    num--;
  }

  EXPECT_EQ(num, i.size());
  EXPECT_EQ(num, i.compressedSize());

  fmt::print(
    "size={}, compressed size={}, ratio={}\n",
    i.size(), i.compressedSize(), i.compression()
  );

  for (int k = 1; k < 10000; k += 2) {
    i.insert(k);
    EXPECT_TRUE(i.exists(k));
    num++;
  }

  EXPECT_EQ(num, i.size());
  EXPECT_EQ(1, i.compressedSize());

  fmt::print(
    "size={}, compressed size={}, ratio={}\n",
    i.size(), i.compressedSize(), i.compression()
  );
}

}}} // end namespace vt::tests::unit
