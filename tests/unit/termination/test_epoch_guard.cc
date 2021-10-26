/*
//@HEADER
// *****************************************************************************
//
//                            test_term_chaining.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include "vt/pipe/pipe_manager.h"
#include "vt/collective/collective_alg.h"
#include "vt/termination/epoch_guard.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestEpochGuard : TestParallelHarness {
  using TestMsg = EpochMessage;
  static EpochType ep;

  static void test_guarded_msg_recv(TestMsg *msg)
  {
    EXPECT_EQ(theMsg()->getEpoch(), ep);
  }

  static void test_guarded_msg_send()
  {
    ep = theTerm()->makeEpochCollective();

    auto guard = epoch_guard( ep );

    auto msg = makeMessage<TestMsg>();
    EXPECT_EQ(theMsg()->getEpoch(), ep);
    auto node = theContext()->getNode();
    if (0 == node) {
      theMsg()->sendMsg<TestMsg, test_guarded_msg_recv>(1, msg);
    }

    guard.finish_epoch();
  }
};

EpochType TestEpochGuard::ep = no_epoch;

TEST_F(TestEpochGuard, test_epoch_guard1) {
  SET_NUM_NODES_CONSTRAINT(2);
  vt::runInEpochCollective( test_guarded_msg_send );
}

}}} // end namespace vt::tests::unit
