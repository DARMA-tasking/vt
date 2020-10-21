/*
//@HEADER
// *****************************************************************************
//
//                           test_signal_cleanup.cc
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

#include <memory>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

using DataMsg = vt::Message;

struct CallbackMsg : vt::Message {
  CallbackMsg() = default;
  explicit CallbackMsg(vt::Callback<DataMsg> in_cb) : cb_(in_cb) { }

  vt::Callback<DataMsg> cb_;
};

using TestSignalCleanup = TestParallelHarness;

void bounce(CallbackMsg* msg) {
  msg->cb_.send<DataMsg>();
}

TEST_F(TestSignalCleanup, test_signal_cleanup_3) {
  auto const this_node = theContext()->getNode();

  int c1 = 0, c2 = 0;

  if (this_node == 0) {
    auto cb = theCB()->makeFunc<DataMsg>(
      vt::pipe::LifetimeEnum::Once, [&c1](DataMsg* msg){
        c1++;
        fmt::print("called A");
      }
    );
    auto msg = makeMessage<CallbackMsg>(cb);
    theMsg()->sendMsg<CallbackMsg, bounce>(1, msg);
  }

  // run until termination
  do vt::runScheduler(); while (not vt::rt->isTerminated());

  // explicitly finalize runtime to destroy and reset components
  vt::rt->finalize(true, false);

  // re-init runtime, fresh state---force it now!
  vt::rt->initialize(true);

  // Create another callback with the same template signature, meaning it will
  // target the same signal holder. The old callback from the previous init
  // should *not* trigger again.
  //
  // Since the RT has been finalized, the pipe ID for the new callback will be
  // the same as the one before.
  //
  if (this_node == 0) {
    auto cb = theCB()->makeFunc<DataMsg>(
      vt::pipe::LifetimeEnum::Once, [&c2](DataMsg* msg){
        c2++;
        fmt::print("called B");
      }
    );
    auto msg = makeMessage<CallbackMsg>(cb);
    theMsg()->sendMsg<CallbackMsg, bounce>(1, msg);
  }

  // run until termination
  do vt::runScheduler(); while (not vt::rt->isTerminated());

  // now, check if we only fired the callbacks exactly once!
  if (this_node == 0) {
    EXPECT_EQ(c1, 1);
    EXPECT_EQ(c2, 1);
  }
}

}}} // end namespace vt::tests::unit
