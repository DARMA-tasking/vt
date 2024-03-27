/*
//@HEADER
// *****************************************************************************
//
//                        test_term_dep_epoch_active.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"
#include "vt/messaging/collection_chain_set.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestTermDepEpochActive : TestParallelHarness { };

struct TestDepActive {
  void depHandler() {
    num_dep++;
    vt_print(gen, "depHandler: num_dep={}, epoch={:x}\n", num_dep, theTerm()->getEpoch());
    EXPECT_EQ(num_non_dep, 1);
  }

  void nonDepHandler() {
    //auto const& node = theContext()->getNode();
    num_non_dep++;
    //fmt::print("{}: nonDepHandler: num_non_dep={}\n", node, num_non_dep);
    EXPECT_EQ(num_dep, 0);
  }

  int num_dep = 0;
  int num_non_dep = 0;
};

TEST_F(TestTermDepEpochActive, test_term_dep_epoch_active) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  int const k = 10;

  auto proxy = theObjGroup()->makeCollective<TestDepActive>("TestDepActive");

  vt::theCollective()->barrier();

  auto epoch = vt::theTerm()->makeEpochCollective(term::ParentEpochCapture{}, true);
  vt::theMsg()->pushEpoch(epoch);
  for (int i = 0; i < k; i++) {
    proxy.broadcast<&TestDepActive::depHandler>();
  }
  vt::theMsg()->popEpoch(epoch);
  vt::theTerm()->finishedEpoch(epoch);

  auto chain = std::make_unique<vt::messaging::CollectionChainSet<NodeType>>();
  chain->addIndex(this_node);

  chain->nextStep([=](NodeType node) {
    return proxy[this_node].send<&TestDepActive::nonDepHandler>();
  });

  chain->nextStep([=](NodeType node) {
    auto msg = vt::makeMessage<vt::Message>();
    return vt::messaging::PendingSend(msg, [=](MsgSharedPtr<vt::BaseMsgType>&){
      EXPECT_EQ(proxy.get()->num_dep, 0);
      proxy[this_node].release(epoch);
    });
  });

  vt::theTerm()->addAction([=]{
    EXPECT_EQ(proxy.get()->num_non_dep, 1);
    EXPECT_EQ(proxy.get()->num_dep, num_nodes * k);
  });
}

}}} // end namespace vt::tests::unit
