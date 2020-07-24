/*
//@HEADER
// *****************************************************************************
//
//                            test_lb.extended.cc
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

#include <vt/transport.h>

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit {

static constexpr int const num_elms = 1024;
static constexpr int const num_phases = 10;

struct MyCol : vt::Collection<MyCol,vt::Index1D> {
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::Collection<MyCol,vt::Index1D>::serialize(s);
    s | val;
  }

  double val = 0.0;
};

using MyMsg = vt::CollectionMessage<MyCol>;

// A dummy kernel that does some work depending on the index
void colHandler(MyMsg*, MyCol* col) {
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < col->getIndex().x() * 20; j++) {
      col->val += (i*29+j*2)-4;
    }
  }
}

struct TestLoadBalancer : TestParallelHarnessParam<std::string> { };

TEST_P(TestLoadBalancer, test_load_balancer_1) {
  auto lb_name = GetParam();

  vt::arguments::ArgConfig::vt_lb = true;
  vt::arguments::ArgConfig::vt_lb_name = lb_name;

  vt::theCollective()->barrier();

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<MyCol> proxy;

  // Construct a collection
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<MyCol>(
      range, [](vt::Index1D){ return std::make_unique<MyCol>(); }
    );
  });

  for (int phase = 0; phase < num_phases; phase++) {
    // Do some work.
    runInEpochCollective([&]{
      auto this_node = vt::theContext()->getNode();
      if (this_node == 0) {
        proxy.broadcast<MyMsg, colHandler>();
      }
    });

    // Go to the next phase.
    runInEpochCollective([&]{
      vt::theCollection()->startPhaseCollective(nullptr);
    });
  }
}

INSTANTIATE_TEST_SUITE_P(
  LoadBalancerExplode, TestLoadBalancer,
  ::testing::Values(
    "RandomLB",
    "RotateLB",
    "HierarchicalLB",
    "GossipLB",
    "GreedyLB"
#   if vt_check_enabled(zoltan)
    , "ZoltanLB"
#   endif
  )
);

}}} // end namespace vt::tests::unit

#endif /*vt_check_enabled(lblite)*/
