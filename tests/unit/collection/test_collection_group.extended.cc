/*
//@HEADER
// *****************************************************************************
//
//                      test_collection_group.extended.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/transport.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit {

static int32_t elem_counter = 0;

struct MyReduceMsg : collective::ReduceTMsg<int> {
  explicit MyReduceMsg(int const in_num)
    : collective::ReduceTMsg<int>(in_num)
  { }
};

struct ColA : Collection<ColA,Index1D> {
  struct TestMsg : CollectionMessage<ColA> { };

  struct TestDataMsg : CollectionMessage<ColA> {
    TestDataMsg(int32_t value) : value_(value) {}

    int32_t value_ = -1;
  };

  void finishedReduce(MyReduceMsg* m) {
    fmt::print("at root: final num={}\n", m->getVal());
    finished = true;
  }

  void doReduce(TestMsg* msg) {
    auto const proxy = getCollectionProxy();
    auto cb = theCB()->makeBcast<ColA, MyReduceMsg, &ColA::finishedReduce>(proxy);
    auto reduce_msg = makeMessage<MyReduceMsg>(getIndex().x());
    proxy.reduce<collective::PlusOp<int>>(reduce_msg.get(),cb);
    reduce_test = true;
  }

  void memberHanlder(TestDataMsg* msg) {
    EXPECT_EQ(msg->value_, theContext()->getNode());
    --elem_counter;
  }

  virtual ~ColA() {
    if (reduce_test) {
      EXPECT_TRUE(finished);
    }
  }

  private:
  bool finished = false;
  bool reduce_test = false;
};

void colHanlder(
  ColA::TestDataMsg* msg, typename ColA::TestDataMsg::CollectionType* type) {
  --elem_counter;
}

struct TestCollectionGroup : TestParallelHarness { };


TEST_F(TestCollectionGroup, test_collection_group_1) {
  auto const my_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  if (my_node == 0) {
    auto const range = Index1D(std::max(num_nodes / 2, 1));
    auto const proxy = theCollection()->construct<ColA>(range);
    proxy.broadcast<ColA::TestMsg,&ColA::doReduce>();
  }
}

TEST_F(TestCollectionGroup, test_collection_group_2){
  auto const my_node = theContext()->getNode();

  auto const range = Index1D(8);
  auto const proxy =
    theCollection()->constructCollective<ColA>(range, [](vt::Index1D idx) {
      ++elem_counter;
      return std::make_unique<ColA>();
    });

  const auto numElems = elem_counter;
  auto msg = ::vt::makeMessage<ColA::TestDataMsg>(my_node);

  proxy.broadcastCollective<ColA::TestDataMsg, &ColA::memberHanlder>(msg.get());
  EXPECT_EQ(elem_counter, 0);

  proxy.broadcastCollective<ColA::TestDataMsg, &ColA::memberHanlder>(msg);
  EXPECT_EQ(elem_counter, -numElems);

  proxy.broadcastCollective<
    ColA::TestDataMsg, &ColA::memberHanlder, ColA::TestDataMsg>(my_node);
  EXPECT_EQ(elem_counter, -2 * numElems);
}

TEST_F(TestCollectionGroup, test_collection_group_3){
  elem_counter = 0;
  auto const my_node = theContext()->getNode();

  auto const range = Index1D(8);
  auto const proxy =
    theCollection()->constructCollective<ColA>(range, [](vt::Index1D idx) {
      ++elem_counter;
      return std::make_unique<ColA>();
    });

  const auto numElems = elem_counter;
  auto msg = ::vt::makeMessage<ColA::TestDataMsg>(my_node);

  proxy.broadcastCollective<ColA::TestDataMsg, colHanlder>(msg.get());
  EXPECT_EQ(elem_counter, 0);

  proxy.broadcastCollective<ColA::TestDataMsg, &ColA::memberHanlder>(msg);
  EXPECT_EQ(elem_counter, -numElems);

  proxy.broadcastCollective<
    ColA::TestDataMsg, &ColA::memberHanlder, ColA::TestDataMsg>(my_node);
  EXPECT_EQ(elem_counter, -2 * numElems);
}

}}} // end namespace vt::tests::unit
