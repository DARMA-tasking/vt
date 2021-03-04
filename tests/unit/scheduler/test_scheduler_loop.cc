/*
//@HEADER
// *****************************************************************************
//
//                          test_scheduler_loop.cc
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

#include "vt/transport.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

struct TestSchedulerLoop : TestParallelHarness { };

struct TestMsg : vt::Message {
  int action_;
};

static bool done = false;

const int action_count = 7;

static std::string actions[action_count] = {
  "-- not used --",
  "nested",
  "nested",
  "manual",
  "manual",
  "nested",
  "done"
};

static bool ack[action_count] = {false};

static void message_handler_with_nested_loop(TestMsg* msg) {
  int depth = msg->action_;
  std::string& action = actions[depth];

  ack[depth] = true;

  if ("done" == action) {
    done = true;
    return;
  }

  // Pass message around the ring..
  NodeType target_node = (theContext()->getNode() + 1) % theContext()->getNumNodes();
  int next_depth = depth + 1;

  auto next_msg = vt::makeMessage<TestMsg>();
  next_msg->action_ = next_depth;
  theMsg()->sendMsg<TestMsg, message_handler_with_nested_loop>(target_node, next_msg);

  // ..run scheduler until someone also passed us the message.
  if ("nested" == action) {
    // New auto-nesting loop (recommended).
    // This also includes the time between the scheduler loops as an event.
    theSched()->runSchedulerWhile([next_depth]{
      return not ack[next_depth];
    });
  } else if ("manual" == action) {
    // Original manual loops.
    // Not recommended as scheduler event generation is less consistent.
    while (not ack[next_depth]) {
      theSched()->scheduler();
    }
  } else {
    vtAssertInfo(false, "Invalid action", action);
  }
}

TEST_F(TestSchedulerLoop, test_scheduler_loop_nesting_1) {

  vtAssert(theContext()->getNumNodes() >= 2, "At least 2 nodes required.");

  NodeType node = theContext()->getNode();
  NodeType target_node = (node + 1) % theContext()->getNumNodes();

  auto msg = vt::makeMessage<TestMsg>();
  msg->action_ = 1;
  theMsg()->sendMsg<TestMsg, message_handler_with_nested_loop>(target_node, msg);

  done = false;
  theSched()->runSchedulerWhile([]{ return not done; });

  SUCCEED();
}

}}} /* end namespace vt::tests::unit */
