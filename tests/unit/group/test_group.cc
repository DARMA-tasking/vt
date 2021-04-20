/*
//@HEADER
// *****************************************************************************
//
//                                test_group.cc
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
#include "data_message.h"
#include "test_helpers.h"

#include "vt/group/group_manager.h"

#include <algorithm>

namespace vt { namespace tests { namespace unit {

using namespace ::vt;
using namespace ::vt::group;
using namespace ::vt::tests::unit;

static int32_t num_recv = 0;

struct TestMsg : ::vt::Message {};

struct TestGroup : TestParallelHarness {
  void SetUp() override {
    TestParallelHarness::SetUp();
    SET_MIN_NUM_NODES_CONSTRAINT(2);
  }

  static void groupHandler(TestMsg* msg) {
    auto const& this_node = theContext()->getNode();
    num_recv++;
    fmt::print("{}: groupHandler: num_recv={}\n", this_node, num_recv);
  }
};

TEST_F(TestGroup, test_group_range_construct_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  NodeType const lo = 0;
  NodeType const hi = num_nodes / 2;

  runInEpochCollective([&]{
    if (this_node == 0) {
      auto list = std::make_unique<region::Range>(lo,hi);
      theGroup()->newGroup(
        std::move(list), [](GroupType group){
          fmt::print("Group is created: group={:x}\n", group);
          auto msg = makeMessage<TestMsg>();
          envelopeSetGroup(msg->env, group);
          theMsg()->broadcastMsg<TestMsg,groupHandler>(msg);
        }
      );
    }
  });

  if (this_node >= lo && this_node < hi) {
    EXPECT_EQ(num_recv, 1);
  } else {
    EXPECT_EQ(num_recv, 0);
  }
  num_recv = 0;
}

TEST_F(TestGroup, test_group_range_construct_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  NodeType const lo = 1;
  NodeType const max_val = 5;
  NodeType const hi = std::min<NodeType>(num_nodes,max_val);


  runInEpochCollective([&]{
    if (this_node == 0) {
      auto list = std::make_unique<region::Range>(lo,hi);
      theGroup()->newGroup(
        std::move(list), [](GroupType group){
          fmt::print("Group is created: group={:x}\n", group);
          auto msg = makeMessage<TestMsg>();
          envelopeSetGroup(msg->env, group);
          theMsg()->broadcastMsg<TestMsg,groupHandler>(msg);
        }
      );
    }
  });

  if (this_node >= lo && this_node < hi) {
    EXPECT_EQ(num_recv, 1);
  } else {
    EXPECT_EQ(num_recv, 0);
  }
  num_recv = 0;
}

TEST_F(TestGroup, test_group_collective_construct_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  bool const node_filter = this_node % 2 == 0;

  runInEpochCollective([&]{
    theGroup()->newGroupCollective(
      node_filter, [=](GroupType group) {
        auto const& in_group = theGroup()->inGroup(group);
        auto const& is_default_group = theGroup()->isGroupDefault(group);
        EXPECT_EQ(in_group, node_filter);
        EXPECT_EQ(is_default_group, false);
        auto msg = makeMessage<TestMsg>();
        envelopeSetGroup(msg->env, group);
        theMsg()->broadcastMsg<TestMsg,groupHandler>(msg);
      }
    );
  });

  if (node_filter) {
    EXPECT_EQ(num_recv, num_nodes);
  } else {
    EXPECT_EQ(num_recv, 0);
  }
  num_recv = 0;
}

}}} // end namespace vt::tests::unit
