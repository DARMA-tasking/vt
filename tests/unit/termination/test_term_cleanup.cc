/*
//@HEADER
// *****************************************************************************
//
//                            test_term_cleanup.cc
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

struct TestTermCleanup :  TestParallelHarness {
  using TestMsgType = TestStaticBytesNormalMsg<64>;

  static void handler(TestMsgType* msg) {
    //fmt::print("recieve msg\n");
  }
};


TEST_F(TestTermCleanup, test_termination_cleanup_1) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  int const num_epochs = 100;

  for (int i = 0; i < num_epochs; i++) {
    EpochType const epoch = theTerm()->makeEpochCollective();
    bool done = false;
    //fmt::print("global collective epoch {:x}\n", epoch);

    NodeType const next = this_node + 1 < num_nodes ? this_node + 1 : 0;

    auto msg = makeMessage<TestMsgType>();
    envelopeSetEpoch(msg->env, epoch);
    theMsg()->sendMsg<TestMsgType, handler>(next, msg.get());

    theTerm()->finishedEpoch(epoch);
    theTerm()->addAction(epoch, [&]{ done = true; });
    do vt::runScheduler(); while (not done);

    EXPECT_LT(theTerm()->getEpochState().size(), 2);
    EXPECT_EQ(theTerm()->getEpochWaitSet().size(), 0);
    EXPECT_EQ(theTerm()->getDSTermMap().size(), 0);
    EXPECT_LT(theTerm()->getEpochReadySet().size(), 2);
  }

  while (not vt::rt->isTerminated() or not vt::theSched()->isIdle()) {
    vt::runScheduler();
  }

  EXPECT_LT(theTerm()->getEpochState().size(), 2);
  EXPECT_EQ(theTerm()->getEpochWaitSet().size(), 0);
  EXPECT_EQ(theTerm()->getDSTermMap().size(), 0);
  EXPECT_LT(theTerm()->getEpochReadySet().size(), 2);
}

TEST_F(TestTermCleanup, test_termination_cleanup_2) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  int const num_epochs = 100;

  for (int i = 0; i < num_epochs; i++) {
    EpochType const coll_epoch = theTerm()->makeEpochCollective();
    EpochType const root_epoch = theTerm()->makeEpochRootedDS(false,no_epoch);
    EpochType const wave_epoch = theTerm()->makeEpochRootedWave(false,no_epoch);
    bool coll_done = false;
    bool root_done = false;
    bool wave_done = false;
    //fmt::print("global collective epoch {:x}\n", epoch);

    NodeType const next = this_node + 1 < num_nodes ? this_node + 1 : 0;

    for (int j = 0; j < 5; j++) {
      auto msg = makeMessage<TestMsgType>();
      envelopeSetEpoch(msg->env, coll_epoch);
      theMsg()->sendMsg<TestMsgType, handler>(next, msg.get());
    }
    for (int j = 0; j < 5; j++) {
      auto msg = makeMessage<TestMsgType>();
      envelopeSetEpoch(msg->env, root_epoch);
      theMsg()->sendMsg<TestMsgType, handler>(next, msg.get());
    }
    for (int j = 0; j < 5; j++) {
      auto msg = makeMessage<TestMsgType>();
      envelopeSetEpoch(msg->env, wave_epoch);
      theMsg()->sendMsg<TestMsgType, handler>(next, msg.get());
    }

    theTerm()->finishedEpoch(coll_epoch);
    theTerm()->finishedEpoch(root_epoch);
    theTerm()->finishedEpoch(wave_epoch);
    theTerm()->addAction(coll_epoch, [&]{ coll_done = true; });
    theTerm()->addAction(root_epoch, [&]{ root_done = true; });
    theTerm()->addAction(wave_epoch, [&]{ wave_done = true; });
    do vt::runScheduler(); while (not coll_done or not root_done or not wave_done);
  }

  while (not vt::rt->isTerminated() or not vt::theSched()->isIdle()) {
    vt::runScheduler();
  }

  EXPECT_LT(theTerm()->getEpochState().size(), 2);
  EXPECT_EQ(theTerm()->getEpochWaitSet().size(), 0);
  EXPECT_EQ(theTerm()->getDSTermMap().size(), 0);
  EXPECT_LT(theTerm()->getEpochReadySet().size(), 2);
}


}}} // end namespace vt::tests::unit
