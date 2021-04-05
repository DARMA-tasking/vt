/*
//@HEADER
// *****************************************************************************
//
//                          test_termination_reset.cc
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
#include "test_helpers.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestTermReset : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static int32_t handler_count;

  static void test_handler(TestMsg* msg) {
    handler_count = 10;
  }
};

/*static*/ int32_t TestTermReset::handler_count = 0;

TEST_F(TestTermReset, test_termination_reset_1) {
  SET_MIN_NUM_NODES_CONSTRAINT(2);

  auto const& this_node = theContext()->getNode();

  if (this_node == 0) {
    auto msg = makeMessage<TestMsg>();
    theMsg()->broadcastMsg<TestMsg, test_handler>(msg);
  } else if (this_node == 1) {
    theTerm()->addAction([=]{
      EXPECT_EQ(handler_count, 10);
    });
  }

  theSched()->runSchedulerWhile([]{ return !rt->isTerminated(); });

  rt->reset();

  handler_count = 0;
  theCollective()->barrier();

  if (this_node == 1) {
    auto msg = makeMessage<TestMsg>();
    theMsg()->broadcastMsg<TestMsg, test_handler>(msg);
  } else if (this_node == 0) {
    theTerm()->addAction([=]{
      EXPECT_EQ(handler_count, 10);
    });
  }
}

}}} // end namespace vt::tests::unit
