/*
//@HEADER
// ************************************************************************
//
//                          term_reset.cc
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

struct TestTermReset : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static int32_t handler_count;

  static void test_handler(TestMsg* msg) {
    handler_count++;
  }
};

/*static*/ int32_t TestTermReset::handler_count = 0;

TEST_F(TestTermReset, test_termination_reset_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNode();

  if (num_nodes < 2) {
    return;
  }

  if (this_node == 0) {
    auto msg = makeSharedMessage<TestMsg>();
    theMsg()->sendMsg<TestMsg, test_handler>(1, msg);
  } else if (this_node == 1) {
    theTerm()->addAction([=]{
      EXPECT_EQ(handler_count, 1);
    });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  rt->reset();

  if (this_node == 1) {
    auto msg = makeSharedMessage<TestMsg>();
    theMsg()->sendMsg<TestMsg, test_handler>(0, msg);
  } else if (this_node == 0) {
    theTerm()->addAction([=]{
      EXPECT_EQ(handler_count, 1);
    });
  }
}

}}} // end namespace vt::tests::unit
