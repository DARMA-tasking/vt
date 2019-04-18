/*
//@HEADER
// ************************************************************************
//
//                          test_active_broadcast.cc
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

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

namespace {
struct CountMsg : ::vt::Message {
  static int alloc_count;

  CountMsg() { ++alloc_count; }
  CountMsg( const CountMsg &other ) { ++alloc_count; }
  CountMsg( CountMsg &&other ) { ++alloc_count; }

  CountMsg &operator=( const CountMsg & ) = default;
  CountMsg &operator=( CountMsg && ) = default;
  
  template< typename Serializer >
  void serialize( Serializer & ) {}

  ~CountMsg() { --alloc_count; }
};

int CountMsg::alloc_count = 0;
}

struct BcastLifetimeRegression : TestParameterHarnessNode {
  using TestMsg = CountMsg;

  static int handler_count;
  static int num_msg_sent;

  virtual void SetUp() {
    TestParameterHarnessNode::SetUp();

    handler_count = 0;
    num_msg_sent = 16;
    TestMsg::alloc_count = 0;
  }

  virtual void TearDown() {
    TestParameterHarnessNode::TearDown();

    EXPECT_EQ(CountMsg::alloc_count, 0);
  }

  static void test_handler(TestMsg* msg) {
    #if DEBUG_TEST_HARNESS_PRINT
      auto const& this_node = theContext()->getNode();
      fmt::print("{}: test_handler: cnt={}\n", this_node, ack_count);
    #endif

    handler_count++;
  }
};

/*static*/ int BcastLifetimeRegression::handler_count;
/*static*/ int BcastLifetimeRegression::num_msg_sent;

TEST_P(BcastLifetimeRegression, bcast_all) {
  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  NodeType const& root = GetParam();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("BcastLifetimeRegression.bcast_all: node={}, root={}\n", my_node, root);
  #endif

  if (root < num_nodes) {
    if (my_node == root) {
      for (int i = 0; i < num_msg_sent; i++) {
        auto msg = makeSharedMessage<TestMsg>();
        theMsg()->broadcastMsgAuto<TestMsg, test_handler>(msg);
      }
    }

    theTerm()->addAction([=]{
      if (my_node != root) {
        ASSERT_TRUE(handler_count == num_msg_sent);
      }
    });
  }
}

INSTANTIATE_TEST_CASE_P(
  InstantiationName, BcastLifetimeRegression,
  ::testing::Range(static_cast<NodeType>(0), static_cast<NodeType>(16), 1)
);

}}} // end namespace vt::tests::unit
