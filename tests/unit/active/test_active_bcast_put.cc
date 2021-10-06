/*
//@HEADER
// *****************************************************************************
//
//                           test_active_bcast_put.cc
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

#include "vt/messaging/active.h"
#include "test_parallel_harness.h"
#include "data_message.h"
#include "test_helpers.h"

namespace vt { namespace tests { namespace unit { namespace bcast_put {

using namespace vt;
using namespace vt::tests::unit;

struct PutTestMessage : ::vt::PayloadMessage {
  PutTestMessage() = default;
};

struct TestActiveBroadcastPut : TestParameterHarnessNode {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static int handler_count;
  static int num_msg_sent;
  static size_t put_size;

  virtual void SetUp() {
    TestParameterHarnessNode::SetUp();
    handler_count = 0;
    num_msg_sent = 1;

    SET_MIN_NUM_NODES_CONSTRAINT(2);
  }

  virtual void TearDown() {
    TestParameterHarnessNode::TearDown();
  }

  static void test_handler(PutTestMessage* msg) {
    #if DEBUG_TEST_HARNESS_PRINT || 1
      auto const& this_node = theContext()->getNode();
      fmt::print("{}: test_handler: cnt={}\n", this_node, handler_count);
    #endif

    auto const ptr = static_cast<int*>(msg->getPut());
    auto const size = msg->getPutSize();

    #if DEBUG_TEST_HARNESS_PRINT || 1
      fmt::print(
        "{}: test_handler: size={}, ptr={}\n", this_node, size, print_ptr(ptr)
      );
    #endif

    EXPECT_EQ(put_size * sizeof(int), size);

    for (size_t i = 0; i < put_size; i++) {
      EXPECT_EQ(static_cast<size_t>(ptr[i]), i + 1);
    }

    handler_count++;
  }
};

/*static*/ int TestActiveBroadcastPut::handler_count;
/*static*/ int TestActiveBroadcastPut::num_msg_sent;
/*static*/ size_t TestActiveBroadcastPut::put_size = 4;

TEST_P(TestActiveBroadcastPut, test_type_safe_active_fn_bcast2) {
  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  NodeType const& root = GetParam();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_bcast: node={}, root={}\n", my_node, root);
  #endif

  std::vector<int> put_payload;

  for (size_t i = 0; i < put_size; i++) {
    put_payload.push_back(i + 1);
  }

  if (root < num_nodes) {
    runInEpochCollective([&]{
      if (my_node == root) {
        for (int i = 0; i < num_msg_sent; i++) {
          auto msg = makeMessage<PutTestMessage>();
          msg->setPut(put_payload.data(), put_size * sizeof(int));
          theMsg()->broadcastMsg<PutTestMessage, test_handler>(msg);
        }
      }
    });

    ASSERT_EQ(num_msg_sent, handler_count);
  }

  // Spin here so test_vec does not go out of scope before the send completes
  vt::theSched()->runSchedulerWhile([]{ return !rt->isTerminated(); });
}

INSTANTIATE_TEST_SUITE_P(
  InstantiationName, TestActiveBroadcastPut,
  ::testing::Range(static_cast<NodeType>(0), static_cast<NodeType>(16), 1)
);

}}}} // end namespace vt::tests::unit::bcast_put
