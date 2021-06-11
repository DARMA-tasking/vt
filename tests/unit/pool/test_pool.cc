/*
//@HEADER
// *****************************************************************************
//
//                                 test_pool.cc
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

#include <cstring>

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include "vt/messaging/active.h"

namespace vt { namespace tests { namespace unit {

using namespace vt::tests::unit;

static constexpr int64_t const min_bytes = 1;
static constexpr int64_t const max_bytes = 16384;

struct TestPool : TestParallelHarness {
  template <int64_t num_bytes>
  using TestMsg = TestStaticBytesShortMsg<num_bytes>;

  template <int64_t num_bytes>
  static void testPoolFun(TestMsg<num_bytes>* prev_msg);

  virtual void SetUp() {
    TestParallelHarness::SetUp();
  }
};

template <int64_t num_bytes>
void TestPool::testPoolFun(TestMsg<num_bytes>* prev_msg) {
  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("testPoolFun: num_bytes={}\n", num_bytes);
  #endif

  {
    auto msg = makeMessage<TestMsg<num_bytes * 2>>();
    testPoolFun<num_bytes * 2>(msg.get());
  }
}

template <>
void TestPool::testPoolFun<max_bytes>(TestMsg<max_bytes>* msg) { }

TEST_F(TestPool, pool_message_alloc) {
  using namespace vt;

  auto const& my_node = theContext()->getNode();

  if (my_node == 0) {
    {
      auto msg = makeMessage<TestMsg<min_bytes>>();
      testPoolFun<min_bytes>(msg.get());
    }
  }
}


TEST_F(TestPool, pool_alloc) {
  using namespace vt;

  using CharType = unsigned char;

  static constexpr CharType const init_val = 'z';

  std::unique_ptr<pool::Pool> testPool = std::make_unique<pool::Pool>();

  for (size_t cur_bytes = 1; cur_bytes < max_bytes; cur_bytes *= 2) {
    void* ptr = testPool->alloc(cur_bytes);
    std::memset(ptr, init_val, cur_bytes);
    //fmt::print("alloc {} bytes, ptr={}\n", cur_bytes, ptr);
    EXPECT_NE(ptr, nullptr);
    for (size_t i = 0; i < cur_bytes; i++) {
      EXPECT_EQ(static_cast<CharType*>(ptr)[i], init_val);
    }
    testPool->dealloc(ptr);
  }
}

}}} // end namespace vt::tests::unit
