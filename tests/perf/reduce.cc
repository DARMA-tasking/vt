/*
//@HEADER
// *****************************************************************************
//
//                                  reduce.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/configs/error/config_assert.h"
#include "vt/context/context.h"
#include <unordered_map>
#include <vt/collective/collective_ops.h>
#include <vt/objgroup/manager.h>
#include <vt/messaging/active.h>
#include <vt/collective/reduce/allreduce/rabenseifner.h>

#include INCLUDE_FMT_CORE

using namespace vt;
using namespace vt::tests::perf::common;

static constexpr int num_iters = 1;

struct MyTest : PerfTestHarness {
  MyTest() { DisableGlobalTimer(); }
};

struct NodeObj {
  explicit NodeObj(MyTest* test_obj) : test_obj_(test_obj) { }

  void initialize() {
    proxy_ = vt::theObjGroup()->getProxy<NodeObj>(this);
    //  data_["Node"] = theContext()->getNode(); }
  }
  struct MyMsg : vt::Message { };

  void newReduceComplete(std::vector<int32_t> in) {
    // fmt::print(
    //   "\n[{}]: allreduce_h done! (Size == {}) Results are ...\n",
    //   theContext()->getNode(), in.size());
    // const auto p = theContext()->getNumNodes();
    // const auto expected = (p * (p + 1)) / 2;
    // for (auto val : in) {
    //   vtAssert(val == expected, "FAILURE!");
    // }
    // for (int node = 0; node < theContext()->getNumNodes(); ++node) {
    //   if (node == theContext()->getNode()) {
    //     std::string printer(128, 0x0);
    //     for (auto val : in) {
    //       printer.append(fmt::format("{} ", val));
    //     }

    //     fmt::print("{}\n", printer);

    //     theCollective()->barrier();
    //   }
    // }

    // fmt::print("\n");
  }

  void reduceComplete(std::vector<int32_t> in) {
    // fmt::print(
    //   "[{}]: allreduce done! Results are ...\n", theContext()->getNode());
    // for (auto val : in) {
    //   fmt::print("{} ", val);
    // }

    // fmt::print("\n");
  }

private:
  MyTest* test_obj_ = nullptr;
  vt::objgroup::proxy::Proxy<NodeObj> proxy_ = {};
};

VT_PERF_TEST(MyTest, test_reduce) {
  auto grp_proxy =
    vt::theObjGroup()->makeCollective<NodeObj>("test_allreduce", this);

  if (theContext()->getNode() == 0) {
    theTerm()->disableTD();
  }

  vt::runInEpochCollective([=] {
    grp_proxy.allreduce<&NodeObj::reduceComplete, collective::PlusOp>(data);
  });

  if (theContext()->getNode() == 0) {
    theTerm()->enableTD();
  }
}

VT_PERF_TEST(MyTest, test_allreduce) {
  auto grp_proxy =
    vt::theObjGroup()->makeCollective<NodeObj>("test_allreduce", this);

  if (theContext()->getNode() == 0) {
    theTerm()->disableTD();
  }

  vt::runInEpochCollective([=] {
    grp_proxy.allreduce_h<&NodeObj::newReduceComplete, collective::PlusOp>(
      data);
  });

  if (theContext()->getNode() == 0) {
    theTerm()->enableTD();
  }
}

VT_PERF_TEST_MAIN()
