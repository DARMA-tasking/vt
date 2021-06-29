/*
//@HEADER
// *****************************************************************************
//
//                          test_callback_func_ctx.cc
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

#include <memory>

namespace vt { namespace tests { namespace unit { namespace func_ctx {

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

struct Context {
  int val = 129;
};

static std::unique_ptr<Context> ctx = nullptr;

static int32_t called = 0;

struct TestCallbackFuncCtx : TestParallelHarness {
  static void test_handler(CallbackDataMsg* msg) {
    auto const& n = theContext()->getNode();
    msg->cb_.send(n+1,n+2,n+3);
  }
};

TEST_F(TestCallbackFuncCtx, test_callback_func_ctx_1) {
  auto const& this_node = theContext()->getNode();
  ctx = std::make_unique<Context>();
  ctx->val = this_node;

  auto cb = theCB()->makeFunc<Context>(
    vt::pipe::LifetimeEnum::Once,
    ctx.get(), [](Context* my_ctx){
      called = 200;
      EXPECT_EQ(my_ctx->val, theContext()->getNode());
    }
  );
  cb.send();
  EXPECT_EQ(called, 200);
}

TEST_F(TestCallbackFuncCtx, test_callback_func_ctx_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  runInEpochCollective([this_node, num_nodes]{
    ctx = std::make_unique<Context>();
    ctx->val = this_node;

    auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    auto cb = theCB()->makeFunc<DataMsg, Context>(
      vt::pipe::LifetimeEnum::Once,
      ctx.get(), [next](DataMsg* msg, Context* my_ctx) {
        called = 500;
        EXPECT_EQ(my_ctx->val, theContext()->getNode());
        // fmt::print("{}: a={},b={},c={}\n",n,msg->a,msg->b,msg->c);
        EXPECT_EQ(msg->a, next + 1);
        EXPECT_EQ(msg->b, next + 2);
        EXPECT_EQ(msg->c, next + 3);
      });
    // fmt::print("{}: next={}\n", this_node, next);
    auto msg = makeMessage<CallbackDataMsg>(cb);
    theMsg()->sendMsg<CallbackDataMsg, test_handler>(next, msg);
  });

  // fmt::print("{}: called={}\n", this_node, called);
  EXPECT_EQ(called, 500);
}

}}}} // end namespace vt::tests::unit::func_ctx
