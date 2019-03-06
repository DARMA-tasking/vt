
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

#include <memory>

namespace vt { namespace tests { namespace unit {

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
    auto nmsg = makeSharedMessage<DataMsg>(n+1,n+2,n+3);
    msg->cb_.send(nmsg);
  }
};

TEST_F(TestCallbackFuncCtx, test_callback_func_ctx_1) {
  auto const& this_node = theContext()->getNode();
  ctx = std::make_unique<Context>();
  ctx->val = this_node;

  auto cb = theCB()->makeFunc<Context>(
    ctx.get(), [](Context* ctx){
      called = 200;
      EXPECT_EQ(ctx->val, theContext()->getNode());
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

  ctx = std::make_unique<Context>();
  ctx->val = this_node;

  auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
  auto cb = theCB()->makeFunc<DataMsg,Context>(
    ctx.get(), [next](DataMsg* msg, Context* ctx){
      auto const& n = theContext()->getNode();
      called = 500;
      EXPECT_EQ(ctx->val, theContext()->getNode());
      //fmt::print("{}: a={},b={},c={}\n",n,msg->a,msg->b,msg->c);
      EXPECT_EQ(msg->a, next+1);
      EXPECT_EQ(msg->b, next+2);
      EXPECT_EQ(msg->c, next+3);
    }
  );
  //fmt::print("{}: next={}\n", this_node, next);
  auto msg = makeSharedMessage<CallbackDataMsg>(cb);
  theMsg()->sendMsg<CallbackDataMsg, test_handler>(next, msg);

  theTerm()->addAction([=]{
    //fmt::print("{}: called={}\n", this_node, called);
    EXPECT_EQ(called, 500);
  });
}

}}} // end namespace vt::tests::unit
