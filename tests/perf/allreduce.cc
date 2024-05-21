/*
//@HEADER
// *****************************************************************************
//
//                                 allreduce.cc
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
#include "vt/collective/collective_alg.h"
#include "vt/collective/reduce/operators/functors/plus_op.h"
#include "vt/configs/error/config_assert.h"
#include "vt/context/context.h"
#include "vt/scheduler/scheduler.h"
#include <unordered_map>
#include <vt/collective/collective_ops.h>
#include <vt/objgroup/manager.h>
#include <vt/messaging/active.h>
#include <vt/collective/reduce/allreduce/rabenseifner.h>
#include <vt/collective/reduce/allreduce/recursive_doubling.h>

#include <fmt-vt/core.h>

using namespace vt;
using namespace vt::tests::perf::common;

struct MyTest : PerfTestHarness {
  void SetUp() override {
    PerfTestHarness::SetUp();
    data.resize(1 << 16);
    for (auto& val : data) {
      val = theContext()->getNode() + 1;
    }
  }

  std::vector<int32_t> data;
};

struct NodeObj {
  explicit NodeObj(MyTest* test_obj, const std::string& name) : test_obj_(test_obj), timer_name_(name) { }

  void initialize() {
    proxy_ = vt::theObjGroup()->getProxy<NodeObj>(this);
    //  data_["Node"] = theContext()->getNode(); }
  }
  struct MyMsg : vt::Message { };

  void recursiveDoubling(std::vector<int32_t> in) {
    // std::string printer(1024, 0x0);
    // printer.append(fmt::format("\n[{}]: recursiveDoubling done! ", theContext()->getNode()));

    // for (int node = 0; node < theContext()->getNumNodes(); ++node) {
    //   if (node == theContext()->getNode()) {

    //     for (auto val : in) {
    //       printer.append(fmt::format("{} ", val));
    //     }

    //     fmt::print("{}\n", printer);

    //     theCollective()->barrier();
    //   }
    // }

    // fmt::print("\n");
    // const auto p = theContext()->getNumNodes();
    // const auto expected = (p * (p + 1)) / 2;
    // for (auto val : in) {
    //   vtAssert(val == expected, "FAILURE!");
    // }
    test_obj_->StopTimer(timer_name_);
  }

  void newReduceComplete(std::vector<int32_t> in) {
    // std::string printer(1024, 0x0);
    // printer.append(fmt::format("\n[{}]: allreduce_rabenseifner done! ", theContext()->getNode()));

    // for (int node = 0; node < theContext()->getNumNodes(); ++node) {
    //   if (node == theContext()->getNode()) {

    //     for (auto val : in) {
    //       printer.append(fmt::format("{} ", val));
    //     }

    //     fmt::print("{}\n", printer);

    //     theCollective()->barrier();
    //   }
    // }

    // fmt::print("\n");
    // const auto p = theContext()->getNumNodes();
    // const auto expected = (p * (p + 1)) / 2;
    // for (auto val : in) {
    //   vtAssert(val == expected, "FAILURE!");
    // }
    test_obj_->StopTimer(timer_name_);
  }

  void reduceComplete(std::vector<int32_t> in) {
    // fmt::print(
    //   "[{}]: allreduce done! Results are ...\n", theContext()->getNode());
    // for (auto val : in) {
    //   fmt::print("{} ", val);
    // }

    // fmt::print("\n");
    test_obj_->StopTimer(timer_name_);
  }

  std::string timer_name_ = {};
  MyTest* test_obj_ = nullptr;
  vt::objgroup::proxy::Proxy<NodeObj> proxy_ = {};
};

VT_PERF_TEST(MyTest, test_reduce) {
  auto grp_proxy =
    vt::theObjGroup()->makeCollective<NodeObj>("test_allreduce", this, "Reduce -> Bcast");

  theCollective()->barrier();
  StartTimer(grp_proxy[theContext()->getNode()].get()->timer_name_);
  grp_proxy.allreduce<&NodeObj::reduceComplete, collective::PlusOp>(data);
}

VT_PERF_TEST(MyTest, test_allreduce_rabenseifner) {
  auto proxy =
    vt::theObjGroup()->makeCollective<NodeObj>("test_allreduce_new", this, "Rabenseifner");

  using DataT = decltype(data);
  using Reducer = collective::reduce::allreduce::Rabenseifner<
    DataT, collective::PlusOp, NodeObj, &NodeObj::newReduceComplete>;

  auto grp_proxy = vt::theObjGroup()->makeCollective<Reducer>(
    "allreduce_rabenseifner", proxy, num_nodes_, data);
  grp_proxy[my_node_].get()->proxy_ = grp_proxy;

  theCollective()->barrier();
  StartTimer(proxy[theContext()->getNode()].get()->timer_name_);
  grp_proxy[my_node_].template invoke<&Reducer::allreduce>();
}

VT_PERF_TEST(MyTest, test_allreduce_recursive_doubling) {
  auto proxy =
    vt::theObjGroup()->makeCollective<NodeObj>("test_allreduce_new_2", this, "Recursive doubling");

  using DataT = decltype(data);
  using Reducer = collective::reduce::allreduce::DistanceDoubling<
    DataT, collective::PlusOp, NodeObj, &NodeObj::recursiveDoubling>;

  auto grp_proxy = vt::theObjGroup()->makeCollective<Reducer>(
    "allreduce_recursive_doubling", proxy, num_nodes_, data);
  grp_proxy[my_node_].get()->proxy_ = grp_proxy;

  theCollective()->barrier();
  StartTimer(proxy[theContext()->getNode()].get()->timer_name_);
  grp_proxy[my_node_].template invoke<&Reducer::allreduce>();
}

VT_PERF_TEST_MAIN()
