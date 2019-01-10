/*
//@HEADER
// ************************************************************************
//
//                          test_group.cc
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
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

#include <algorithm>

namespace vt { namespace tests { namespace unit {

using namespace ::vt;
using namespace ::vt::group;
using namespace ::vt::tests::unit;

static int32_t num_recv = 0;

struct TestMsg : ::vt::Message {};

struct TestGroup : TestParallelHarness {
  static void groupHandler(TestMsg* msg) {
    auto const& this_node = theContext()->getNode();
    num_recv++;
    fmt::print("{}: groupHandler: num_recv={}\n", this_node, num_recv);
  }
};

TEST_F(TestGroup, test_group_range_construct_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& lo = 0;
  auto const& hi = num_nodes / 2;
  if (this_node == 0) {
    auto list = std::make_unique<region::Range>(lo,hi);
    auto const& group_id = theGroup()->newGroup(
      std::move(list), [](GroupType group){
        fmt::print("Group is created: group={:x}\n", group);
        auto msg = makeSharedMessage<TestMsg>();
        envelopeSetGroup(msg->env, group);
        theMsg()->broadcastMsg<TestMsg,groupHandler>(msg);
      }
    );
  }
  theTerm()->addAction([=]{
    if (this_node >= lo && this_node < hi) {
      EXPECT_EQ(num_recv, 1);
    } else {
      EXPECT_EQ(num_recv, 0);
    }
    num_recv = 0;
  });
}

TEST_F(TestGroup, test_group_range_construct_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& lo = 1;
  auto const& hi = std::min(num_nodes,static_cast<NodeType>(5));
  if (this_node == 0) {
    auto list = std::make_unique<region::Range>(lo,hi);
    auto const& group_id = theGroup()->newGroup(
      std::move(list), [](GroupType group){
        fmt::print("Group is created: group={:x}\n", group);
        auto msg = makeSharedMessage<TestMsg>();
        envelopeSetGroup(msg->env, group);
        theMsg()->broadcastMsg<TestMsg,groupHandler>(msg);
      }
    );
  }
  theTerm()->addAction([=]{
    if (this_node >= lo && this_node < hi) {
      EXPECT_EQ(num_recv, 1);
    } else {
      EXPECT_EQ(num_recv, 0);
    }
    num_recv = 0;
  });
}

TEST_F(TestGroup, test_group_collective_construct_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& node_filter = this_node % 2 == 0;
  theGroup()->newGroupCollective(
    node_filter, [=](GroupType group) {
      auto const& in_group = theGroup()->inGroup(group);
      auto const& root_node = theGroup()->groupRoot(group);
      auto const& is_default_group = theGroup()->groupDefault(group);
      EXPECT_EQ(in_group, node_filter);
      EXPECT_EQ(is_default_group, false);
      auto msg = makeSharedMessage<TestMsg>();
      envelopeSetGroup(msg->env, group);
      theMsg()->broadcastMsg<TestMsg,groupHandler>(msg);
    }
  );
  theTerm()->addAction([=]{
    if (node_filter) {
      EXPECT_EQ(num_recv, num_nodes);
    } else {
      EXPECT_EQ(num_recv, 0);
    }
    num_recv = 0;
  });
}

// TEST_F(TestGroup, test_group_collective_construct_2) {
//   auto const& this_node = theContext()->getNode();
//   auto const& num_nodes = theContext()->getNumNodes();
//   auto const& node_filter = this_node % 2 == 1;
//   theGroup()->newGroupCollective(
//     node_filter, [=](GroupType group) {
//       auto const& in_group = theGroup()->inGroup(group);
//       auto const& root_node = theGroup()->groupRoot(group);
//       auto const& is_default_group = theGroup()->groupDefault(group);
//       ::fmt::print("{}: new group collective lambda\n", this_node);
//       EXPECT_EQ(in_group, node_filter);
//       EXPECT_EQ(is_default_group, false);
//       auto msg = makeSharedMessage<TestMsg>();
//       envelopeSetGroup(msg->env, group);
//       theMsg()->broadcastMsg<TestMsg,groupHandler>(msg);
//     }
//   );
//   theTerm()->addAction([=]{
//     if (node_filter) {
//       EXPECT_EQ(num_recv, num_nodes);
//     } else {
//       EXPECT_EQ(num_recv, 0);
//     }
//     num_recv = 0;
//   });
// }

}}} // end namespace vt::tests::unit
