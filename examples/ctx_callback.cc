/*
//@HEADER
// *****************************************************************************
//
//                               ctx_callback.cc
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

#include "vt/transport.h"

#include <cstdlib>
#include <cassert>

using namespace ::vt;

struct MyContext {
  int x = 29;
};

struct DataMsg : Message {
  using MessageParentType = Message;
  vt_msg_serialize_required(); // by vec_

  DataMsg() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
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
}

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
