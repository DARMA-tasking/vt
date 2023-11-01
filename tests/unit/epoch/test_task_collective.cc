/*
//@HEADER
// *****************************************************************************
//
//                           test_task_collective.cc
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

#include <cstring>

#include <gtest/gtest.h>
#include <vt/transport.h>
#include <vt/messaging/collection_chain_set.h>

#include "test_harness.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

using TestTaskCollective = TestParallelHarness;

struct TestGroup {

  void task1(int val) {
    vt_print(gen, "val={}, t1={}\n", val, t1);
    t1++;
  }

  void task2(int val) {
    vt_print(gen, "val={}, t1={}, t2={}\n", val, t1, t2);
    t2++;
    EXPECT_EQ(t1, t2);
  }

  void task3(int val) {
    vt_print(gen, "val={}, t1={}, t2={}, t3={}\n", val, t1, t2, t3);
    t3++;
    EXPECT_EQ(t1, t2);
    EXPECT_EQ(t2, t3);
  }

  void setProxy(objgroup::proxy::Proxy<TestGroup> in_proxy) {
    proxy_ = in_proxy;
  }

private:
  int t1 = 0, t2 = 0, t3 = 0, t4 = 0;
  objgroup::proxy::Proxy<TestGroup> proxy_;
};

TEST_F(TestTaskCollective, test_node_task_collective_1) {
  using ChainSetType = messaging::CollectionChainSet<NodeType>;

  auto const num_nodes = theContext()->getNumNodes();

  auto proxy = theObjGroup()->makeCollective<TestGroup>("TestGroup");
  proxy.get()->setProxy(proxy);

  auto chains_ = std::make_unique<ChainSetType>();
  chains_->addIndex(theContext()->getNode());

  auto tr = chains_->createTaskRegion([&]{
    auto t1 = chains_->taskCollective("task1", [&](auto node, auto t) {
      return proxy[node].template send<&TestGroup::task1>(10);
    });

    auto t2 = chains_->taskCollective("task2", [&](auto node, auto t) {
      t->dependsOn(node, t1);
      t->dependsOn((node+1)%num_nodes, t1);
      return proxy[node].template send<&TestGroup::task2>(10);
    });

    /*auto t3 = */chains_->taskCollective("task3", [&](auto node, auto t) {
      t->dependsOn(node, t2);
      return proxy[node].template send<&TestGroup::task3>(10);
    });
  });

  // Do this a bunch of times
  for (int i = 0; i < 100; i++) {
    tr->enqueueTasks();
    tr->waitCollective();
  }
}

}}} // end namespace vt::tests::unit
