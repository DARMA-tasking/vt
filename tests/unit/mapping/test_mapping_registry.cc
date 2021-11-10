/*
//@HEADER
// *****************************************************************************
//
//                           test_mapping_registry.cc
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

#include "vt/topos/index/index.h"
#include "vt/topos/mapping/mapping.h"
#include "vt/topos/mapping/dense/dense.h"
#include "vt/registry/auto/map/auto_registry_map.h"

#include <vector>

namespace vt { namespace tests { namespace unit {

struct TestMsg : vt::ShortMessage {
  bool is_block = false;
  HandlerType han;
  TestMsg(HandlerType const in_han) : ShortMessage(), han(in_han) { }
};

struct TestMappingRegistry : TestParallelHarness {
  static void test_handler(TestMsg* msg) {
    //auto const& this_node = theContext()->getNode();
    //fmt::print("{}: test_handler: han={}\n", this_node, msg->han);
    auto fn = auto_registry::getAutoHandlerMap(msg->han);

    static constexpr Index1D::DenseIndexType const val = 64;
    static constexpr Index1D::DenseIndexType const max = 256;
    static constexpr NodeType const nnodes = 8;

    Index1D idx(val);
    Index1D max_idx(max);

    auto const node = fn->dispatch(&idx, &max_idx, nnodes);

    if (msg->is_block) {
      EXPECT_EQ(val / (max / nnodes), node);
    } else {
      EXPECT_EQ(val % nnodes, node);
    }
    //fmt::print("node={}\n", node);
  }

};

static NodeType map_fn(
  Index1D* idx, Index1D* max_idx, NodeType nnodes
) {
  return mapping::dense1DBlockMap(idx, max_idx, nnodes);
}

static NodeType map_fn2(
  Index1D* idx, Index1D* max_idx, NodeType nnodes
) {
  return mapping::dense1DRoundRobinMap(idx, max_idx, nnodes);
}

TEST_F(TestMappingRegistry, test_mapping_block_1d_registry) {
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_send: node={}\n", my_node);
  #endif

  if (my_node == 0) {
    auto map_han = auto_registry::makeAutoHandlerMap<Index1D, map_fn>();
    auto msg = makeMessage<TestMsg>(map_han);
    msg->is_block = true;
    theMsg()->broadcastMsg<TestMsg, test_handler>(msg);

    auto map_han2 = auto_registry::makeAutoHandlerMap<Index1D, map_fn2>();
    auto msg2 = makeMessage<TestMsg>(map_han2);
    theMsg()->broadcastMsg<TestMsg, test_handler>(msg2);
  }
}

}}} // end namespace vt::tests::unit
