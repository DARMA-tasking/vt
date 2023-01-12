/*
//@HEADER
// *****************************************************************************
//
//                                test_atomic.cc
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
#include "test_parallel_harness.h"
#include "vt/utils/atomic/atomic.h"
#include "vt/utils/mutex/mutex.h"

#include <thread>

namespace vt { namespace tests { namespace unit {

using namespace vt::tests::unit;

using ::vt::util::atomic::AtomicType;

struct TestAtomic : TestHarness { };

static constexpr int const num_vals = 128;
static constexpr int const init_val = 29;

TEST_F(TestAtomic, basic_atomic_init_single_thd) {
  AtomicType<int> test1 = {init_val};
  EXPECT_EQ(test1.load(), init_val);
}

TEST_F(TestAtomic, basic_atomic_load_single_thd) {
  AtomicType<int> test1 = {init_val};
  EXPECT_EQ(test1.load(), init_val);
  auto val = test1.load();
  EXPECT_EQ(val, init_val);
}

TEST_F(TestAtomic, basic_atomic_store_single_thd) {
  AtomicType<int> test1 = {init_val};
  EXPECT_EQ(test1.load(), init_val);
  auto const store_val = init_val + 10;
  test1.store(store_val);
  auto val = test1.load();
  EXPECT_EQ(val, store_val);
}

TEST_F(TestAtomic, basic_atomic_fetch_add_single_thd) {
  AtomicType<int> test1 = {0};
  for (int i = 0; i < num_vals; i++) {
    auto val = test1.fetch_add(1);
    EXPECT_EQ(val, i);
  }
}

TEST_F(TestAtomic, basic_atomic_fetch_sub_single_thd) {
  AtomicType<int> test1 = {0};
  for (int i = 0; i < num_vals; i++) {
    auto val = test1.fetch_sub(1);
    EXPECT_EQ(val, -i);
  }
}

}}} // end namespace vt::tests::unit
