/*
//@HEADER
// *****************************************************************************
//
//                             test_pending_send.cc
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

struct TestPendingSend : TestParallelHarness {
  struct TestMsg : vt::Message { };
  static void handlerPong(TestMsg*) { delivered = true; }
  static void handlerPing(TestMsg*) {
    auto const this_node = theContext()->getNode();
    auto const num_nodes = theContext()->getNumNodes();
    auto prev = this_node - 1 >= 0 ? this_node - 1 : num_nodes - 1;
    auto msg = vt::makeMessage<TestMsg>();
    theMsg()->sendMsg<TestMsg, handlerPong>(prev, msg);
  }

  static bool delivered;
};

/*static*/ bool TestPendingSend::delivered = false;

TEST_F(TestPendingSend, test_pending_send_hold) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  delivered = false;

  std::vector<messaging::PendingSend> pending;
  auto ep = theTerm()->makeEpochCollective();
  theMsg()->pushEpoch(ep);

  auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;

  auto msg = vt::makeMessage<TestMsg>();
  auto msg_hold = promoteMsg(msg.get());
  pending.emplace_back(
    theMsg()->sendMsg<TestMsg, handlerPing>(next, msg)
  );

  // Must be stamped with the current epoch
  EXPECT_EQ(envelopeGetEpoch(msg_hold->env), ep);

  theMsg()->popEpoch(ep);
  theTerm()->finishedEpoch(ep);

  // It should not break out of this loop because of
  // !theTerm()->isEpochTermianted(ep), thus `k` is used to
  // break out
  int k = 0;

  theSched()->runSchedulerWhile([&k, ep] {
    return !theTerm()->isEpochTerminated(ep) && (++k <= 10);
  });

  // Epoch should not end with a valid pending send created in an live epoch
  EXPECT_EQ(theTerm()->isEpochTerminated(ep), false);
  EXPECT_EQ(delivered, false);

  // Now we send the message off!
  pending.clear();

  vt::runSchedulerThrough(ep);

  EXPECT_EQ(theTerm()->isEpochTerminated(ep), true);
  EXPECT_EQ(delivered, true);
}

}}} // end namespace vt::tests::unit
