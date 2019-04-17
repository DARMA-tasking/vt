/*
//@HEADER
// ************************************************************************
//
//                          test_active_send.cc
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

struct SendLifetimeRegression : TestParallelHarness {
  using TestMsg = CountMsg;

  static NodeType from_node;
  static NodeType to_node;

  static int handler_count;
  static int num_msg_sent;

  virtual void SetUp() {
    TestParallelHarness::SetUp();

    handler_count = 0;
    num_msg_sent = 16;

    from_node = 0;
    to_node = 1;
  }

  virtual void TearDown() {
    TestParallelHarness::TearDown();

    EXPECT_EQ(CountMsg::alloc_count, 0);
  }

  static void test_handler(TestMsg* msg) {
    auto const& this_node = theContext()->getNode();

    #if DEBUG_TEST_HARNESS_PRINT
      fmt::print("{}: test_handler: cnt={}\n", this_node, ack_count);
    #endif

    handler_count++;

    EXPECT_EQ(this_node, to_node);
  }
};

/*static*/ NodeType SendLifetimeRegression::from_node;
/*static*/ NodeType SendLifetimeRegression::to_node;
/*static*/ int SendLifetimeRegression::handler_count;
/*static*/ int SendLifetimeRegression::num_msg_sent;

TEST_F(SendLifetimeRegression, send_one_dest) {
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("send_one_dest: node={}\n", my_node);
  #endif

  if (my_node == from_node) {
    for (int i = 0; i < num_msg_sent; i++) {
      #if DEBUG_TEST_HARNESS_PRINT
        fmt::print("{}: sendMsg: i={}\n", my_node, i);
      #endif
      auto msg = makeSharedMessage<TestMsg>();
      theMsg()->sendMsg<TestMsg, test_handler>(1, msg);
    }
  } else if (my_node == to_node) {
    theTerm()->addAction([=]{
      EXPECT_EQ(handler_count, num_msg_sent);
    });
  }
}


}}} // end namespace vt::tests::unit
