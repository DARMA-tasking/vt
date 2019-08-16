/*
//@HEADER
// ************************************************************************
//
//                 test_term_dep_epoch_objgroup.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"
#include "vt/messaging/collection_chain_set.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestTermDepEpochObjGroup : TestParallelHarness { };

struct TestMsg : vt::Message { };

struct TestDep {
  void depHandler(TestMsg* msg) {
    //auto const& node = theContext()->getNode();
    num_dep++;
    //fmt::print("{}: depHandler: num_dep={}\n", node, num_dep);
    EXPECT_EQ(num_non_dep, 1);
  }

  void nonDepHandler(TestMsg* msg) {
    //auto const& node = theContext()->getNode();
    num_non_dep++;
    //fmt::print("{}: nonDepHandler: num_non_dep={}\n", node, num_non_dep);
    EXPECT_EQ(num_dep, 0);
  }

  int num_dep = 0;
  int num_non_dep = 0;
};

TEST_F(TestTermDepEpochObjGroup, test_term_dep_epoch_objgroup) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  bool const bcast = true;
  int const k = 10;

  auto proxy = vt::theObjGroup()->makeCollective<TestDep>();
  vt::theCollective()->barrier();

  auto epoch = vt::theTerm()->makeEpochCollectiveDep();
  vt::theMsg()->pushEpoch(epoch);
  if (bcast) {
    for (int i = 0; i < k; i++) {
      auto msg = vt::makeSharedMessage<TestMsg>();
      proxy.template broadcast<TestMsg, &TestDep::depHandler>(msg);
    }
  } else {
  }
  vt::theMsg()->popEpoch(epoch);
  vt::theTerm()->finishedEpoch(epoch);

  auto chain = std::make_unique<vt::messaging::CollectionChainSet<NodeType>>();
  chain->addIndex(this_node);

  chain->nextStep([=](NodeType node) {
    auto msg = vt::makeMessage<TestMsg>();
    return vt::messaging::PendingSend(msg, [=](MsgVirtualPtr<vt::BaseMsgType>){
      auto const next = this_node + 1 < num_nodes ? this_node + 1 : 0;
      auto msg2 = vt::makeSharedMessage<TestMsg>();
      proxy[next].send<TestMsg, &TestDep::nonDepHandler>(msg2);
    });
  });

  chain->nextStep([=](NodeType node) {
    auto msg = vt::makeMessage<TestMsg>();
    return vt::messaging::PendingSend(msg, [=](MsgVirtualPtr<vt::BaseMsgType>){
      EXPECT_EQ(proxy[node].get()->num_dep, 0);
      proxy[node].release(epoch);
    });
  });

  vt::theTerm()->addAction([=]{
    auto const node = vt::theContext()->getNode();
    EXPECT_EQ(proxy[node].get()->num_non_dep, 1);
    EXPECT_EQ(proxy[node].get()->num_dep, num_nodes*k);
  });
}

}}} // end namespace vt::tests::unit
