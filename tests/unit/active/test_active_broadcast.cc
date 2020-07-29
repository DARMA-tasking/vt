/*
//@HEADER
// *****************************************************************************
//
//                           test_active_broadcast.cc
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

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestActiveBroadcast : TestParameterHarnessNode {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static int handler_count;
  static int num_msg_sent;

  virtual void SetUp() {
    TestParameterHarnessNode::SetUp();

    handler_count = 0;
    num_msg_sent = 16;
  }

  virtual void TearDown() {
    TestParameterHarnessNode::TearDown();
  }

  static void test_handler(TestMsg* msg) {
    #if DEBUG_TEST_HARNESS_PRINT
      auto const& this_node = theContext()->getNode();
      fmt::print("{}: test_handler: cnt={}\n", this_node, ack_count);
    #endif

    handler_count++;
  }
};

/*static*/ int TestActiveBroadcast::handler_count;
/*static*/ int TestActiveBroadcast::num_msg_sent;

TEST_P(TestActiveBroadcast, test_type_safe_active_fn_bcast2) {
  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  NodeType const& root = GetParam();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_bcast: node={}, root={}\n", my_node, root);
  #endif

  if (root < num_nodes) {
    runInEpochCollective([&]{
      if (my_node == root) {
        for (int i = 0; i < num_msg_sent; i++) {
          auto msg = makeMessage<TestMsg>();
          theMsg()->broadcastMsg<TestMsg, test_handler>(msg.get());
        }
      }
    });

    if (my_node != root) {
      ASSERT_TRUE(handler_count == num_msg_sent);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
  InstantiationName, TestActiveBroadcast,
  ::testing::Range(static_cast<NodeType>(0), static_cast<NodeType>(16), 1)
);

}}} // end namespace vt::tests::unit
