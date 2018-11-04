
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"

#include "vt/transport.h"
#include "topos/index/index.h"
#include "topos/mapping/mapping.h"
#include "registry/auto/map/auto_registry_map.h"

#include <vector>

namespace vt { namespace tests { namespace unit {

struct TestMsg : vt::ShortMessage {
  bool is_block = false;
  HandlerType han;
  TestMsg(HandlerType const& in_han) : ShortMessage(), han(in_han) { }
};

struct TestMappingRegistry : TestParallelHarness {
  static void test_handler(TestMsg* msg) {
    auto const& this_node = theContext()->getNode();
    //fmt::print("{}: test_handler: han={}\n", this_node, msg->han);
    auto fn = auto_registry::getAutoHandlerMap(msg->han);

    static constexpr index::Index1D::DenseIndexType const val = 64;
    static constexpr index::Index1D::DenseIndexType const max = 256;
    static constexpr NodeType const nnodes = 8;

    index::Index1D idx(val);
    index::Index1D max_idx(max);

    auto const& node = fn(
      reinterpret_cast<index::BaseIndex*>(&idx),
      reinterpret_cast<index::BaseIndex*>(&max_idx),
      nnodes
    );

    if (msg->is_block) {
      EXPECT_EQ(val / (max / nnodes), node);
    } else {
      EXPECT_EQ(val % nnodes, node);
    }
    //fmt::print("node={}\n", node);
  }

};

static NodeType map_fn(
  index::Index1D* idx, index::Index1D* max_idx, NodeType nnodes
) {
  return mapping::dense1DBlockMap(idx, max_idx, nnodes);
}

static NodeType map_fn2(
  index::Index1D* idx, index::Index1D* max_idx, NodeType nnodes
) {
  return mapping::dense1DRoundRobinMap(idx, max_idx, nnodes);
}

TEST_F(TestMappingRegistry, test_mapping_block_1d_registry) {
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_send: node={}\n", my_node);
  #endif

  if (my_node == 0) {
    auto map_han = auto_registry::makeAutoHandlerMap<index::Index1D, map_fn>();
    auto msg = new TestMsg(map_han);
    msg->is_block = true;
    theMsg()->broadcastMsg<TestMsg, test_handler>(msg, [=]{ delete msg; });

    auto map_han2 = auto_registry::makeAutoHandlerMap<index::Index1D, map_fn2>();
    auto msg2 = new TestMsg(map_han2);
    theMsg()->broadcastMsg<TestMsg, test_handler>(msg2, [=]{ delete msg2; });
  }
}

}}} // end namespace vt::tests::unit
