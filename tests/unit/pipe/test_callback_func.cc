/*
//@HEADER
// *****************************************************************************
//
//                            test_callback_func.cc
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

struct CallbackMsg : vt::Message {
  CallbackMsg() = default;
  explicit CallbackMsg(Callback<> in_cb) : cb_(in_cb) { }

  Callback<> cb_;
};

static int32_t called = 0;

struct TestCallbackFunc : TestParallelHarness {
  static void test_handler(CallbackMsg* msg) {
    //auto const& this_node = theContext()->getNode();
    //fmt::print("{}: test_handler\n", this_node);
    msg->cb_.send();
  }
};

TEST_F(TestCallbackFunc, test_callback_func_1) {
  called = 0;
  auto cb = theCB()->makeFunc(vt::pipe::LifetimeEnum::Once, []{ called = 900; });
  cb.send();
  EXPECT_EQ(called, 900);
}

TEST_F(TestCallbackFunc, test_callback_func_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  called = 0;

  runInEpochCollective([this_node]{
    if (this_node == 0) {
      auto cb = theCB()->makeFunc(
        vt::pipe::LifetimeEnum::Once, []{ called = 400; }
      );
      auto msg = makeMessage<CallbackMsg>(cb);
      theMsg()->sendMsg<CallbackMsg, test_handler>(1, msg);
    }
  });

  if (this_node == 0) {
    EXPECT_EQ(called, 400);
  }
}

}}} // end namespace vt::tests::unit
