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
#include "test_helpers.h"

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
  SET_MIN_NUM_NODES_CONSTRAINT(2);

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  int const num_epochs = 100;

  for (int i = 0; i < num_epochs; i++) {
    EpochType const epoch = theTerm()->makeEpochCollective();
    //fmt::print("global collective epoch {:x}\n", epoch);

    NodeType const next = this_node + 1 < num_nodes ? this_node + 1 : 0;

    auto msg = makeMessage<TestMsgType>();
    envelopeSetEpoch(msg->env, epoch);
    theMsg()->sendMsg<TestMsgType, handler>(next, msg);

    theTerm()->finishedEpoch(epoch);
    vt::runSchedulerThrough(epoch);

    EXPECT_LT(theTerm()->getEpochState().size(), std::size_t{2});
    EXPECT_EQ(theTerm()->getEpochWaitSet().size(), std::size_t{0});
    EXPECT_EQ(theTerm()->getDSTermMap().size(), std::size_t{0});
    EXPECT_LT(theTerm()->getEpochReadySet().size(), std::size_t{2});
  }

  theSched()->runSchedulerWhile(
    [] { return not vt::rt->isTerminated() or not vt::theSched()->isIdle();
  });

  EXPECT_LT(theTerm()->getEpochState().size(), std::size_t{2});
  EXPECT_EQ(theTerm()->getEpochWaitSet().size(), std::size_t{0});
  EXPECT_EQ(theTerm()->getDSTermMap().size(), std::size_t{0});
  EXPECT_LT(theTerm()->getEpochReadySet().size(), std::size_t{2});
}

TEST_F(TestTermCleanup, test_termination_cleanup_2) {
  SET_MIN_NUM_NODES_CONSTRAINT(2);

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  int const num_epochs = 100;

  for (int i = 0; i < num_epochs; i++) {
    EpochType const coll_epoch = theTerm()->makeEpochCollective();
    EpochType const root_epoch = theTerm()->makeEpochRootedDS(
      term::SuccessorEpochCapture{no_epoch}
    );
    EpochType const wave_epoch = theTerm()->makeEpochRootedWave(
      term::SuccessorEpochCapture{no_epoch}
    );
    //fmt::print("global collective epoch {:x}\n", epoch);

    NodeType const next = this_node + 1 < num_nodes ? this_node + 1 : 0;

    for (int j = 0; j < 5; j++) {
      auto msg = makeMessage<TestMsgType>();
      envelopeSetEpoch(msg->env, coll_epoch);
      theMsg()->sendMsg<TestMsgType, handler>(next, msg);
    }
    for (int j = 0; j < 5; j++) {
      auto msg = makeMessage<TestMsgType>();
      envelopeSetEpoch(msg->env, root_epoch);
      theMsg()->sendMsg<TestMsgType, handler>(next, msg);
    }
    for (int j = 0; j < 5; j++) {
      auto msg = makeMessage<TestMsgType>();
      envelopeSetEpoch(msg->env, wave_epoch);
      theMsg()->sendMsg<TestMsgType, handler>(next, msg);
    }

    theTerm()->finishedEpoch(coll_epoch);
    theTerm()->finishedEpoch(root_epoch);
    theTerm()->finishedEpoch(wave_epoch);
    vt::runSchedulerThrough(coll_epoch);
    vt::runSchedulerThrough(root_epoch);
    vt::runSchedulerThrough(wave_epoch);
  }

  vt::theSched()->runSchedulerWhile(
    []{ return not vt::rt->isTerminated() or not vt::theSched()->isIdle();
  });

  EXPECT_LT(theTerm()->getEpochState().size(), std::size_t{2});
  EXPECT_EQ(theTerm()->getEpochWaitSet().size(), std::size_t{0});
  EXPECT_EQ(theTerm()->getDSTermMap().size(), std::size_t{0});
  EXPECT_LT(theTerm()->getEpochReadySet().size(), std::size_t{2});
}


}}} // end namespace vt::tests::unit
