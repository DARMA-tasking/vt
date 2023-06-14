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

struct DataMsg : vt::Message {
  DataMsg() = default;
  DataMsg(int in_a, int in_b, int in_c) : a(in_a), b(in_b), c(in_c) { }
  int a = 0, b = 0, c = 0;
};

static int32_t called = 0;

template <typename... Ts>
void applyCallback(vt::Callback<Ts...> cb, Ts... ts) {
  cb.send(ts...);
}

template <typename MsgT, typename... Ts>
void applyCallbackMsg(vt::Callback<MsgT> cb, Ts... ts) {
  cb.send(ts...);
}

using TestCallbackBcast = TestParallelHarness;

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
    auto cb = theCB()->makeBcast<callbackFn>();
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
    NodeType next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<callbackFn>();
    theMsg()->send<applyCallbackMsg<DataMsg, int, int, int>>(
      vt::Node{next}, cb, 1, 2, 3
    );
  });

  EXPECT_EQ(called, 100 * theContext()->getNumNodes());
}

TEST_F(TestCallbackBcast, test_callback_bcast_remote_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  theCollective()->barrier();

  runInEpochCollective([&]{
    NodeType next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<CallbackFunctor>();
    theMsg()->send<applyCallbackMsg<DataMsg, int, int, int>>(
      vt::Node{next}, cb, 1, 2, 3
    );
  });

  EXPECT_EQ(called, 200 * theContext()->getNumNodes());
}

TEST_F(TestCallbackBcast, test_callback_bcast_remote_3) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  theCollective()->barrier();

  runInEpochCollective([&]{
    NodeType next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<CallbackFunctorEmpty>();
    theMsg()->send<applyCallback<>>(vt::Node{next}, cb);
  });

  EXPECT_EQ(called, 300 * theContext()->getNumNodes());
}

static void callbackVoidFn() {
  called = 100;
}

static void callbackParamFn(int a, int b) {
  EXPECT_EQ(a, 10);
  EXPECT_EQ(b, 20);
  called = 100;
}

static void callbackParamSerFn(std::string a, int b) {
  EXPECT_EQ(a, "hello");
  EXPECT_EQ(b, 20);
  called = 100;
}

struct CallbackVoidFn {
  void operator()() {
    called = 100;
  }
};

struct CallbackParamFn {
  void operator()(int a, int b) {
    EXPECT_EQ(a, 10);
    EXPECT_EQ(b, 20);
    called = 100;
  }
};

struct CallbackParamSerFn {
  void operator()(std::string a, int b) {
    EXPECT_EQ(a, "hello");
    EXPECT_EQ(b, 20);
    called = 100;
  }
};

TEST_F(TestCallbackBcast, test_callback_bcast_param_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  runInEpochCollective([this_node, num_nodes]{
    NodeType next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<callbackVoidFn>();
    theMsg()->send<applyCallback<>>(vt::Node{next}, cb);
  });

  EXPECT_EQ(called, 100);
}

TEST_F(TestCallbackBcast, test_callback_bcast_param_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  runInEpochCollective([this_node, num_nodes]{
    NodeType next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<callbackParamFn>();
    theMsg()->send<applyCallback<int, int>>(vt::Node{next}, cb, 10, 20);
  });

  EXPECT_EQ(called, 100);
}

TEST_F(TestCallbackBcast, test_callback_bcast_param_3) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  runInEpochCollective([this_node, num_nodes]{
    NodeType next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<callbackParamSerFn>();
    theMsg()->send<applyCallback<std::string, int>>(vt::Node{next}, cb, "hello", 20);
  });

  EXPECT_EQ(called, 100);
}

TEST_F(TestCallbackBcast, test_callback_bcast_param_4) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  runInEpochCollective([this_node, num_nodes]{
    NodeType next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<CallbackVoidFn>();
    theMsg()->send<applyCallback<>>(vt::Node{next}, cb);
  });

  EXPECT_EQ(called, 100);
}

TEST_F(TestCallbackBcast, test_callback_bcast_param_5) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  runInEpochCollective([this_node, num_nodes]{
    NodeType next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<CallbackParamFn>();
    theMsg()->send<applyCallback<int, int>>(vt::Node{next}, cb, 10, 20);
  });

  EXPECT_EQ(called, 100);
}

TEST_F(TestCallbackBcast, test_callback_bcast_param_6) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  called = 0;

  runInEpochCollective([this_node, num_nodes]{
    NodeType next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeBcast<CallbackParamSerFn>();
    theMsg()->send<applyCallback<std::string, int>>(vt::Node{next}, cb, "hello", 20);
  });

  EXPECT_EQ(called, 100);
}

}}}} // end namespace vt::tests::unit::bcast
