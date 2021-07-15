/*
//@HEADER
// *****************************************************************************
//
//                       test_reduce_collection_race.cc
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

#include <vt/transport.h>
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace race {

using TestReduceCollectionRace = TestParallelHarnessParam<int>;

struct MyCol : vt::Collection<MyCol, vt::Index1D> {};
struct TestMsg : vt::CollectionMessage<MyCol> {};
using ReduceMsg = vt::collective::ReduceTMsg<int>;

static int multipler = 0;

struct ReduceFunctor {
  void operator()(ReduceMsg* msg) {
    auto const num_nodes = theContext()->getNumNodes();
    auto const num_elems = num_nodes * multipler;
    fmt::print("reduce finished: val={}, num_elems={}\n", msg->getVal(), num_elems);
    EXPECT_EQ(msg->getVal(), (num_elems * (num_elems-1))/2);
  }
};

static void handler(TestMsg*, MyCol* col) {
  auto proxy = col->getCollectionProxy();
  auto msg = vt::makeMessage<ReduceMsg>(static_cast<int>(col->getIndex().x()));
  auto cb = vt::theCB()->makeSend<ReduceFunctor>(0);
  proxy.reduce<vt::collective::PlusOp<int>>(msg.get(), cb);
}

TEST_P(TestReduceCollectionRace, test_reduce_race_1) {
  auto const num_nodes = theContext()->getNumNodes();

  multipler = GetParam();
  auto const range = Index1D(multipler * num_nodes);
  auto proxy = theCollection()->constructCollective<MyCol>(range);

  proxy.broadcastCollective<TestMsg, &handler>();
  proxy.broadcastCollective<TestMsg, &handler>();
  proxy.broadcastCollective<TestMsg, &handler>();
  proxy.broadcastCollective<TestMsg, &handler>();
  proxy.broadcastCollective<TestMsg, &handler>();
}

INSTANTIATE_TEST_SUITE_P(
  InstantiationName, TestReduceCollectionRace, ::testing::Range(1, 5)
);

}}}} // end namespace vt::tests::unit::race
