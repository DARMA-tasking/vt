/*
//@HEADER
// *****************************************************************************
//
//                            test_callback_bcast.cc
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

#include "vt/pipe/pipe_manager.h"
#include "vt/collective/collective_alg.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace bcast {

using namespace vt;
using namespace vt::tests::unit;

struct CallbackMsg : vt::Message {
  CallbackMsg() = default;
  explicit CallbackMsg(Callback<> in_cb) : cb_(in_cb) { }

  Callback<> cb_;
};

struct DataMsg : vt::Message {
  DataMsg() = default;
  DataMsg(int in_a, int in_b, int in_c) : a(in_a), b(in_b), c(in_c) { }
  int a = 0, b = 0, c = 0;
};

struct CallbackDataMsg : vt::Message {
  CallbackDataMsg() = default;
  explicit CallbackDataMsg(Callback<DataMsg> in_cb) : cb_(in_cb) { }

  Callback<DataMsg> cb_;
};

static int32_t called = 0;

struct TestCallbackBcast : TestParallelHarness {
  static void testHandler(CallbackDataMsg* msg) {
    msg->cb_.send(1,2,3);
  }
  static void testHandlerEmpty(CallbackMsg* msg) {
    msg->cb_.send();
  }
};

static void callbackFn(DataMsg* msg) {
  EXPECT_EQ(msg->a, 1);
  EXPECT_EQ(msg->b, 2);
  EXPECT_EQ(msg->c, 3);
  called += 100;
}

struct CallbackFunctor {
  void operator()(DataMsg* msg) {
    EXPECT_EQ(msg->a, 1);
    EXPECT_EQ(msg->b, 2);
    EXPECT_EQ(msg->c, 3);
    called += 200;
  }
};

struct CallbackFunctorEmpty {
  void operator()() {
    called += 300;
  }
};

TEST_F(TestCallbackBcast, test_callback_bcast_1) {
  called = 0;

  theCollective()->barrier();

  runInEpochCollective([&]{
    auto cb = theCB()->makeBcast<DataMsg,callbackFn>();
    cb.send(1,2,3);
  });

  EXPECT_EQ(called, 100 * theContext()->getNumNodes());
}

TEST_F(TestCallbackBcast, test_callback_bcast_2) {
  called = 0;

  theCollective()->barrier();

  runInEpochCollective([&]{
    auto cb = theCB()->makeBcast<CallbackFunctor>();
    cb.send(1,2,3);
  });

  EXPECT_EQ(called, 200 * theContext()->getNumNodes());
}

TEST_F(TestCallbackBcast, test_callback_bcast_3) {
  called = 0;

  theCollective()->barrier();

  runInEpochCollective([&]{
    auto cb = theCB()->makeBcast<CallbackFunctorEmpty>();
    cb.send();
  });

  EXPECT_EQ(called, 300 * theContext()->getNumNodes());
}

TEST_F(TestCallbackBcast, test_callback_bcast_remote_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  theCollective()->barrier();

  runInEpochCollective([&]{
    auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<DataMsg,callbackFn>();
    auto msg = makeMessage<CallbackDataMsg>(cb);
    theMsg()->sendMsg<testHandler>(next, msg);
  });

  EXPECT_EQ(called, 100 * theContext()->getNumNodes());
}

TEST_F(TestCallbackBcast, test_callback_bcast_remote_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  theCollective()->barrier();

  runInEpochCollective([&]{
    auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<CallbackFunctor>();
    auto msg = makeMessage<CallbackDataMsg>(cb);
    theMsg()->sendMsg<testHandler>(next, msg);
  });

  EXPECT_EQ(called, 200 * theContext()->getNumNodes());
}

TEST_F(TestCallbackBcast, test_callback_bcast_remote_3) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  theCollective()->barrier();

  runInEpochCollective([&]{
    auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<CallbackFunctorEmpty>();
    auto msg = makeMessage<CallbackMsg>(cb);
    theMsg()->sendMsg<testHandlerEmpty>(next, msg);
  });

  EXPECT_EQ(called, 300 * theContext()->getNumNodes());
}


}}}} // end namespace vt::tests::unit::bcast
