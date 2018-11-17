
#include "vt/transport.h"

#include <cstdlib>
#include <cassert>

using namespace ::vt;

struct MyContext {
  int x = 29;
};

struct DataMsg : Message {
  DataMsg() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | vec_;
  }

  std::vector<int> vec_;
};

struct Msg : Message {
  Msg() = default;
  explicit Msg(Callback<DataMsg> in_cb) : cb(in_cb) { }

  Callback<DataMsg> cb;
};

static void callback_fn(DataMsg* msg, MyContext* ctx) {
  ::fmt::print("callback_fn: msg={}, ctx={}\n", print_ptr(msg), print_ptr(ctx));
  ::fmt::print("callback_fn: x={}, vec.size={}\n", ctx->x, msg->vec_.size());
  for (auto&& elm : msg->vec_) {
    ::fmt::print("\t elm={}\n", elm);
  }
};

static void handler(Msg* msg) {
  auto cb = msg->cb;
  auto data_msg = makeSharedMessage<DataMsg>();
  data_msg->vec_ = std::vector<int>{18,45,28,-1,344};
  ::fmt::print("handler: vec.size={}\n", data_msg->vec_.size());
  cb.send(data_msg);
}

static MyContext my_global_ctx = {};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (this_node == 0) {
    my_global_ctx.x = 1283;

    auto cb = theCB()->makeFunc<DataMsg,MyContext>(&my_global_ctx, callback_fn);
    auto msg = makeSharedMessage<Msg>(cb);
    theMsg()->sendMsg<Msg,handler>(1, msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
