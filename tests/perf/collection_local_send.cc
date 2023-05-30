/*
//@HEADER
// *****************************************************************************
//
//                           collection_local_send.cc
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
#include <vt/transport.h>

#include <fmt-vt/core.h>

using namespace vt;
using namespace vt::tests::perf::common;

static constexpr int num_iters = 1000000;

struct MyTest : PerfTestHarness { };
struct MyMsg : vt::Message {};

struct NodeObj;
vt::objgroup::proxy::Proxy<NodeObj> global_proxy;

void dummyHandler(MyMsg*) {}

struct TestCol : vt::Collection<TestCol, vt::Index1D> {
  using ColMsg = vt::CollectionMessage<TestCol>;
  void han(ColMsg* msg) {}
};

struct NodeObj {
  struct ReduceMsg : vt::collective::ReduceNoneMsg { };

  explicit NodeObj(MyTest* test_obj) : test_obj_(test_obj) { }
  void initialize() {
    proxy_ = global_proxy = vt::theObjGroup()->getProxy<NodeObj>(this);

    auto range = vt::Index1D(8);
    col_proxy = vt::makeCollection<TestCol>("test")
      .bounds(range)
      .bulkInsert()
      .wait();
    // fmt::print("ptr={}\n", print_ptr(col_proxy.lm));
  }

  void complete() {
  }

  void perfMakeRunnable(MyMsg* in_msg) {
    theTerm()->disableTD();

    test_obj_->StartTimer(fmt::format("colSend {}", num_iters));
    perfRunBenchmark();
    test_obj_->StopTimer(fmt::format("colSend {}", num_iters));

    theTerm()->enableTD();
  }

  void perfRunBenchmark() {
    for (int i = 0; i < num_iters; i++) {
      auto m = makeMessage<TestCol::ColMsg>();
      col_proxy[0].template sendMsg<&TestCol::han>(m);
      vt::theSched()->runSchedulerOnceImpl();
    }
  }

private:
  std::vector<MsgSharedPtr<TestCol::ColMsg>> msgs;
  HandlerType han;
  MyTest* test_obj_ = nullptr;
  vt::objgroup::proxy::Proxy<NodeObj> proxy_ = {};
  vt::CollectionProxy<TestCol> col_proxy;
  int reduce_counter_ = -1;
  int i = 0;
};

VT_PERF_TEST(MyTest, test_collection_local_send) {
  auto grp_proxy = vt::theObjGroup()->makeCollective<NodeObj>(
    "test_collection_local_send", this
  );

  grp_proxy[my_node_].invoke<&NodeObj::initialize>();

  if (theContext()->getNode() == 0) {
    grp_proxy[my_node_].send<&NodeObj::perfMakeRunnable>();
  }
}

VT_PERF_TEST_MAIN()
