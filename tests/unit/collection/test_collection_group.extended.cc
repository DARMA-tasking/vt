/*
//@HEADER
// *****************************************************************************
//
//                      test_collection_group.extended.cc
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
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/vrt/collection/manager.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit {

static int32_t elem_counter = 0;

struct MyReduceMsg : collective::ReduceTMsg<int> {
  explicit MyReduceMsg(int const in_num)
    : collective::ReduceTMsg<int>(in_num)
  { }
};

struct ColA : Collection<ColA,Index1D> {

  struct TestDataMsg : CollectionMessage<ColA> {
    TestDataMsg(int32_t value) : value_(value) {}

    int32_t value_ = -1;
  };

  void finishedReduce(MyReduceMsg* m) {
    fmt::print("at root: final num={}\n", m->getVal());
    finished = true;
  }

  void doReduce() {
    auto const proxy = getCollectionProxy();
    auto cb = theCB()->makeBcast<ColA, MyReduceMsg, &ColA::finishedReduce>(proxy);
    auto reduce_msg = makeMessage<MyReduceMsg>(getIndex().x());
    proxy.reduce<collective::PlusOp<int>>(reduce_msg.get(),cb);
    reduce_test = true;
  }

  void memberHandler(TestDataMsg* msg) {
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

void colHandler(typename ColA::TestDataMsg::CollectionType* type, ColA::TestDataMsg*) {
  --elem_counter;
}

template <typename f>
void runBcastTestHelper(f&& func) {
  runInEpochCollective([=]{
    func();
  });
}

struct TestCollectionGroup : TestParallelHarness { };

TEST_F(TestCollectionGroup, test_collection_group_1) {
  auto const my_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  if (my_node == 0) {
    auto const range = Index1D(std::max(num_nodes / 2, 1));
    auto const proxy = theCollection()->construct<ColA>(
      range, "test_collection_group_1"
    );
    proxy.broadcast<&ColA::doReduce>();
  }
}

TEST_F(TestCollectionGroup, test_collection_group_2) {
  auto const my_node = theContext()->getNode();

  auto const range = Index1D(8);
  auto const proxy = theCollection()->constructCollective<ColA>(
    range, [](vt::Index1D idx) {
      ++elem_counter;
      return std::make_unique<ColA>();
    }, "test_collection_group_2"
  );

  auto const numElems = elem_counter;

  // raw msg pointer case
  runBcastTestHelper([proxy, my_node]{
    auto msg = ::vt::makeMessage<ColA::TestDataMsg>(my_node);
    proxy.broadcastCollectiveMsg<ColA::TestDataMsg, &ColA::memberHandler>(
      msg.get()
    );
  });

  EXPECT_EQ(elem_counter, 0);

  // smart msg pointer case
  runBcastTestHelper([proxy, my_node]{
    auto msg = ::vt::makeMessage<ColA::TestDataMsg>(my_node);
    proxy.broadcastCollectiveMsg<ColA::TestDataMsg, &ColA::memberHandler>(msg);
  });

  EXPECT_EQ(elem_counter, -numElems);

  // msg constructed on the fly case
  runBcastTestHelper([proxy, my_node] {
    proxy.broadcastCollective<
      ColA::TestDataMsg, &ColA::memberHandler, ColA::TestDataMsg>(my_node);
  });

  EXPECT_EQ(elem_counter, -2 * numElems);
}

TEST_F(TestCollectionGroup, test_collection_group_3) {
  elem_counter = 0;
  auto const my_node = theContext()->getNode();

  auto const range = Index1D(8);
  auto const proxy = theCollection()->constructCollective<ColA>(
    range, [](vt::Index1D idx) {
      ++elem_counter;
      return std::make_unique<ColA>();
    }, "test_collection_group_3"
  );

  auto const numElems = elem_counter;

  // raw msg pointer case
  runBcastTestHelper([proxy, my_node]{
    auto msg = ::vt::makeMessage<ColA::TestDataMsg>(my_node);
    proxy.broadcastCollectiveMsg<ColA::TestDataMsg, colHandler>(msg.get());
  });

  EXPECT_EQ(elem_counter, 0);

  // smart msg pointer case
  runBcastTestHelper([proxy, my_node]{
    auto msg = ::vt::makeMessage<ColA::TestDataMsg>(my_node);
    proxy.broadcastCollectiveMsg<ColA::TestDataMsg, colHandler>(msg);
  });

  EXPECT_EQ(elem_counter, -numElems);

  // msg constructed on the fly case
  runBcastTestHelper([proxy, my_node]{
    proxy.broadcastCollective<ColA::TestDataMsg, colHandler, ColA::TestDataMsg>(
      my_node
    );
  });

  EXPECT_EQ(elem_counter, -2 * numElems);
}

template <typename ColT>
struct TestCollectionMsg : CollectionMessage<ColT> {
  using MessageParentType = CollectionMessage<ColT>;
  vt_msg_serialize_required();

  explicit TestCollectionMsg(NodeType const node): node_{node} {
    ::fmt::print("Creating TestCollectionMsg on node: {}\n", node_);
  }

  TestCollectionMsg() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);

    s | node_;

    if (s.isPacking() || s.isUnpacking()) {
      was_serialized_ = true;
    }
  }

  NodeType fromNode() const { return node_; }

  bool wasSerialized() const { return was_serialized_; }

private:
  NodeType node_{-1};
  bool was_serialized_{false};
};

struct TestCollection : Collection<TestCollection, Index1D> {
  void handleBroadcastMsg(TestCollectionMsg<TestCollection>* msg) {
    auto const from_node = msg->fromNode();
    auto const to_node = theContext()->getNode();
    auto const was_serialized = msg->wasSerialized();

    ::fmt::print(
      "handleBroadcastMsg(): from node: {}, to node: {}, msg was serialized: {}\n",
      from_node, to_node, was_serialized
    );

    // Even if msg was sent locally, it is still serialized,
    // and the handler gots a fresh copy of it.
    if (from_node == to_node) {
      EXPECT_TRUE(was_serialized);
    }
  }

  void handleInvokeMsg(TestCollectionMsg<TestCollection>* msg) {
    auto const from_node = msg->fromNode();
    auto const to_node = theContext()->getNode();
    auto const index = getIndex();
    auto const was_serialized = msg->wasSerialized();

    ::fmt::print(
      "handleInvokeMsg(): from node: {}, to node: {}, index: {}, msg was serialized: {}\n",
      from_node, to_node, index, was_serialized
    );

    // invoke() shouldn't serialize message.
    if (from_node == to_node) {
      EXPECT_FALSE(was_serialized);
    }
  }
};

TEST_F(TestCollectionGroup, test_collection_group_serialize_when_broadcast) {
  auto const range = Index1D{static_cast<int>(theContext()->getNumNodes())};
  auto const proxy =
    makeCollection<TestCollection>("test_collection_group_serialize_when_broadcast")
      .bounds(range)
      .bulkInsert()
      .wait();

  // Broadcast from each node
  runInEpochCollective([proxy] {
    auto const this_node = theContext()->getNode();
    auto msg = ::vt::makeMessage<TestCollectionMsg<TestCollection>>(this_node);

    proxy.broadcastMsg<
      TestCollectionMsg<TestCollection>, &TestCollection::handleBroadcastMsg
    >(msg.get());
  });
}

TEST_F(TestCollectionGroup, test_collection_group_dont_serialize_when_invoke) {
  auto const range = Index1D{static_cast<int>(theContext()->getNumNodes())};
  auto const proxy = makeCollection<TestCollection>("test_collection_group_dont_serialize_when_invoke")
    .bounds(range)
    .bulkInsert()
    .wait();

  runInEpochCollective([proxy] {
    auto const this_node = theContext()->getNode();
    proxy[this_node].invoke<
      TestCollectionMsg<TestCollection>, &TestCollection::handleInvokeMsg
    >(this_node);
  });
}

}}} // end namespace vt::tests::unit
