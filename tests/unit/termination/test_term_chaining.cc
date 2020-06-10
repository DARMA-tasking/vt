/*
//@HEADER
// *****************************************************************************
//
//                            test_term_chaining.cc
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
#include "vt/messaging/dependent_send_chain.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestTermChaining : TestParallelHarness {
  using TestMsg = EpochMessage;

  static int32_t handler_count;

  static vt::messaging::DependentSendChain chain;
  static vt::EpochType epoch;

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

    EXPECT_EQ(handler_count, 12);
    handler_count++;
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
    EpochType epoch1 = theTerm()->makeEpochRooted();
    vt::theMsg()->pushEpoch(epoch1);
    auto msg = makeSharedMessage<TestMsg>();
    chain.add(epoch1, theMsg()->sendMsg<TestMsg, test_handler_reflector>(1, msg));
    vt::theMsg()->popEpoch(epoch1);
    vt::theTerm()->finishedEpoch(epoch1);

    EpochType epoch2 = theTerm()->makeEpochRooted();
    vt::theMsg()->pushEpoch(epoch2);
    auto msg2 = makeSharedMessage<TestMsg>();
    chain.add(epoch2, theMsg()->sendMsg<TestMsg, test_handler_chainer>(1, msg2));
    vt::theMsg()->popEpoch(epoch2);
    vt::theTerm()->finishedEpoch(epoch2);

    chain.done();
  }
};

/*static*/ int32_t TestTermChaining::handler_count = 0;
/*static*/ vt::messaging::DependentSendChain TestTermChaining::chain;
/*static*/ vt::EpochType TestTermChaining::epoch;

TEST_F(TestTermChaining, test_termination_chaining_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes != 2)
    return;

  epoch = theTerm()->makeEpochCollective();

  fmt::print("global collective epoch {:x}\n", epoch);

  if (this_node == 0) {
    theMsg()->pushEpoch(epoch);
    start_chain();
    theTerm()->finishedEpoch(epoch);
    theMsg()->popEpoch(epoch);
    fmt::print("before run 1\n");
    vt::runSchedulerThrough(epoch);
    fmt::print("after run 1\n");

    EXPECT_EQ(handler_count, 4);
  } else {
    theMsg()->pushEpoch(epoch);
    theTerm()->finishedEpoch(epoch);
    theMsg()->popEpoch(epoch);
    vt::runSchedulerThrough(epoch);
    EXPECT_EQ(handler_count, 13);
  }
}

}}} // end namespace vt::tests::unit
