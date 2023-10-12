/*
//@HEADER
// ************************************************************************
//
//                   test_term_dep_epoch_active.cc
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

struct TestTermDepEpochActive : TestParallelHarness { };

struct TestDep {
  static void depHandler() {
    num_dep++;
    vt_print(gen, "depHandler: num_dep={}, epoch={:x}\n", num_dep, theTerm()->getEpoch());
    EXPECT_EQ(num_non_dep, 1);
  }

  static void nonDepHandler() {
    //auto const& node = theContext()->getNode();
    num_non_dep++;
    //fmt::print("{}: nonDepHandler: num_non_dep={}\n", node, num_non_dep);
    EXPECT_EQ(num_dep, 0);
  }

  static int num_dep;
  static int num_non_dep;
};

/*static*/ int TestDep::num_dep = 0;
/*static*/ int TestDep::num_non_dep = 0;

TEST_F(TestTermDepEpochActive, test_term_dep_epoch_active) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  int const k = 10;

  TestDep::num_dep = 0;
  TestDep::num_non_dep = 0;
  vt::theCollective()->barrier();

  auto epoch = vt::theTerm()->makeEpochCollective(term::ParentEpochCapture{}, true);
  vt::theMsg()->pushEpoch(epoch);
  for (int i = 0; i < k; i++) {
    vt::theMsg()->broadcast<TestDep::depHandler>();
  }
  vt::theMsg()->popEpoch(epoch);
  vt::theTerm()->finishedEpoch(epoch);

  auto chain = std::make_unique<vt::messaging::CollectionChainSet<NodeType>>();
  chain->addIndex(this_node);

  chain->nextStep([=](NodeType node) {
    NodeType const next = this_node + 1 < num_nodes ? this_node + 1 : 0;
    return vt::theMsg()->send<TestDep::nonDepHandler>(Node{next});
  });

  chain->nextStep([=](NodeType node) {
    auto msg = vt::makeMessage<vt::Message>();
    return vt::messaging::PendingSend(msg, [=](MsgSharedPtr<vt::BaseMsgType>&){
      EXPECT_EQ(TestDep::num_dep, 0);
      theTerm()->releaseEpoch(epoch);
    });
  });

  vt::theTerm()->addAction([=]{
    EXPECT_EQ(TestDep::num_non_dep, 1);
    EXPECT_EQ(TestDep::num_dep, num_nodes * k);
  });
}

}}} // end namespace vt::tests::unit
