/*
//@HEADER
// *****************************************************************************
//
//                               ping_pong_am.cc
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

#include "common/test_harness.h"
#include <vt/collective/collective_ops.h>
#include <vt/objgroup/manager.h>
#include <vt/messaging/active.h>

#include <fmt-vt/core.h>

using namespace vt;
using namespace vt::tests::perf::common;

static constexpr int num_iters = 10000;
static int i = 0;

struct MyTest : PerfTestHarness { };

struct MyMsg : vt::Message {};

struct NodeObj;

vt::objgroup::proxy::Proxy<NodeObj> global_proxy;

void handlerFinished(MyMsg* msg);

void handler(MyMsg* in_msg) {
  auto msg = makeMessage<MyMsg>();
  theMsg()->sendMsg<&handlerFinished>(0, msg);
}

struct NodeObj {
  struct ReduceMsg : vt::collective::ReduceNoneMsg { };

  explicit NodeObj(MyTest* test_obj) : test_obj_(test_obj) { }

  void initialize() {
    proxy_ = global_proxy = vt::theObjGroup()->getProxy<NodeObj>(this);
  }

  void complete() {
    test_obj_->StopTimer(fmt::format("{} ping-pong", i));
    if (theContext()->getNode() == 0) {
      theTerm()->enableTD();
    }
  }

  void perfPingPong(MyMsg* in_msg) {
    test_obj_->StartTimer(fmt::format("{} ping-pong", i));
    auto msg = makeMessage<MyMsg>();
    theMsg()->sendMsg<&handler>(1, msg);
  }

private:
  MyTest* test_obj_ = nullptr;
  vt::objgroup::proxy::Proxy<NodeObj> proxy_ = {};
  int reduce_counter_ = -1;
  int i = 0;
};

void handlerFinished(MyMsg* msg) {
  if (i >= num_iters) {
    global_proxy[0].invoke<&NodeObj::complete>();
  } else {
    i++;
    auto msg = makeMessage<MyMsg>();
    theMsg()->sendMsg<&handler>(1, msg);
  }
}

VT_PERF_TEST(MyTest, test_ping_pong_am) {
  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj>(
    "test_ping_pong_am", this
  );

  if (theContext()->getNode() == 0) {
    theTerm()->disableTD();
  }

  grp_proxy[my_node_].invoke<&NodeObj::initialize>();

  if (theContext()->getNode() == 0) {
    grp_proxy[my_node_].send<MyMsg, &NodeObj::perfPingPong>();
  }
}

VT_PERF_TEST_MAIN()
