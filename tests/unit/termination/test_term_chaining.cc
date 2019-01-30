/*
//@HEADER
// ************************************************************************
//
//                          term_chaining.cc
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
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestTermChaining : TestParallelHarness {
  using TestMsg = EpochMessage;

  static int32_t handler_count;

  static vt::messaging::PendingSend pending;

  static void test_handler_reflector(TestMsg* msg) {
    fmt::print("reflector run\n");

    handler_count = 12;

    EXPECT_EQ(theContext()->getNode(), 1);
    auto msg2 = makeSharedMessage<TestMsg>();
    theMsg()->sendMsg<TestMsg, test_handler_response>(0, msg2);
  }

  static void test_handler_response(TestMsg* msg) {
    fmt::print("response run\n");

    EXPECT_EQ(theContext()->getNode(), 0);
    EXPECT_EQ(handler_count, 0);
    handler_count++;
  }

  static void test_handler_chainer(TestMsg* msg) {
    fmt::print("chainer run\n");

    handler_count = 13;
    auto msg2 = makeSharedMessage<TestMsg>();
    theMsg()->sendMsg<TestMsg, test_handler_chained>(0, msg2);
  }

  static void test_handler_chained(TestMsg* msg) {
    fmt::print("chained run\n");

    EXPECT_EQ(theContext()->getNode(), 0);
    EXPECT_EQ(handler_count, 1);
    handler_count = 4;
  }

  static void start_chain() {
    auto msg = makeSharedMessage<TestMsg>();
    pending = theMsg()->sendMsg<TestMsg, test_handler_reflector>(1, msg);

    pending.chainNextSend([&]{
	fmt::print("nextsend run\n");

	EXPECT_EQ(handler_count, 1);
	auto msg2 = makeSharedMessage<TestMsg>();
	return theMsg()->sendMsg<TestMsg, test_handler_chainer>(1, msg2);
      });
    theTerm()->finishedEpoch(pending.getEpoch());
  }

  static void run_to_term(bool last = false) {
    while (!rt->isTerminated()) {
      runScheduler();
    }
    if (!last) {
      rt->reset();
    }
  }
};

/*static*/ int32_t TestTermChaining::handler_count = 0;
/*static*/ vt::messaging::PendingSend TestTermChaining::pending{nullptr};

TEST_F(TestTermChaining, test_termination_chaining_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes != 2)
    return;

  if (this_node == 0) {
    start_chain();
    fmt::print("before run 1\n");
    run_to_term();
    fmt::print("after run 1\n");
    theTerm()->finishedEpoch(pending.getEpoch());
    //pending.release();
    fmt::print("before run 2\n");
    run_to_term(true);
    fmt::print("after run 2\n");

    EXPECT_EQ(handler_count, 4);
  } else {
    run_to_term();
    EXPECT_EQ(handler_count, 12);
    run_to_term(true);
    EXPECT_EQ(handler_count, 13);
  }
}

}}} // end namespace vt::tests::unit
