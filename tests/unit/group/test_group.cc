
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
