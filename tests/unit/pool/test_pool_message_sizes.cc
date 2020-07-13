/*
//@HEADER
// *****************************************************************************
//
//                          test_pool_message_sizes.cc
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

#include <cstring>

#include <gtest/gtest.h>

#include "test_harness.h"
#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

static constexpr int64_t const min_bytes = 1;
static constexpr int64_t const max_bytes = 16384;
static constexpr int const max_test_count = 1024;
static constexpr NodeType const from_node = 0;
static constexpr NodeType const to_node = 1;

struct TestPoolMessageSizes : TestParallelHarness {
  static int count;

  virtual void SetUp() {
    TestParallelHarness::SetUp();
    count = 0;
  }

  template <int64_t num_bytes>
  using TestMsg = TestStaticBytesShortMsg<num_bytes>;

  template <int64_t num_bytes>
  static void testPoolFun(TestMsg<num_bytes>* msg);

  template <int64_t num_bytes>
  void initData(TestMsg<num_bytes>* msg, vt::tests::unit::ByteType const& val) {
    for (auto& elm : msg->payload) {
      elm = val;
    }
  }
};

/*static*/ int TestPoolMessageSizes::count;

template <int64_t num_bytes>
void TestPoolMessageSizes::testPoolFun(TestMsg<num_bytes>* prev_msg) {
  auto const& this_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("{}: test: bytes={}, cnt={}\n", this_node, num_bytes, count);
  #endif

  count++;

  NodeType const next =
    this_node == from_node ? to_node : from_node;

  if (count < max_test_count) {
    auto msg = makeMessage<TestMsg<num_bytes>>();
    theMsg()->sendMsg<TestMsg<num_bytes>, testPoolFun<num_bytes>>(
      next, msg.get()
    );
  } else {
    auto msg = makeMessage<TestMsg<num_bytes * 2>>();
    theMsg()->sendMsg<TestMsg<num_bytes * 2>, testPoolFun<num_bytes * 2>>(
      next, msg.get()
    );
    count = 0;
  }
}

template <>
void TestPoolMessageSizes::testPoolFun<max_bytes>(TestMsg<max_bytes>* msg) { }

TEST_F(TestPoolMessageSizes, pool_message_sizes_alloc) {
  using namespace vt;

  auto const& my_node = theContext()->getNode();

  if (my_node == 0) {
    auto msg = makeMessage<TestMsg<min_bytes>>();
    theMsg()->sendMsg<TestMsg<min_bytes>, testPoolFun>(
      to_node, msg.get()
    );
  }
}

}}} // end namespace vt::tests::unit
