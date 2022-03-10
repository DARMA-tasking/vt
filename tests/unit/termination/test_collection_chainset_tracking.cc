/*
//@HEADER
// *****************************************************************************
//
//                     test_collection_chainset_tracking.cc
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
#include <vt/messaging/collection_chain_set.h>
#include <gtest/gtest.h>

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace termination {

struct MyCol : Collection<MyCol, Index3D> { };

using TestCollectionChainsetTracking = TestParallelHarness;

TEST_F(TestCollectionChainsetTracking, test_local_chainset_tracking) {
  auto num_nodes = theContext()->getNumNodes();
  auto range = Index3D(2, static_cast<int>(num_nodes), 3);
  auto proxy = vt::makeCollection<MyCol>()
    .collective(true)
    .bounds(range)
    .bulkInsert()
    .wait();
  auto cs = std::make_unique<messaging::CollectionChainSet<Index3D>>(proxy);

  theCollective()->barrier();

  auto local_set = theCollection()->getLocalIndices(proxy);
  auto cs_set = std::set<Index3D>{cs->getSet().begin(), cs->getSet().end()};
  EXPECT_EQ(local_set, cs_set);
  cs->phaseDone();

  theCollective()->barrier();

  auto getNthPosition = [&](Index3D idx, int n) -> NodeType {
    auto const home = theCollection()->getMappedNode(proxy, idx);
    auto num = vt::theContext()->getNumNodes();
    return (home + n) % num;
  };

  for (int i = 1; i < num_nodes + 1; i++) {
    runInEpochCollective("doMigrations", [&]{
      for (auto&& idx : local_set) {
        auto ptr = proxy[idx].tryGetLocalPtr();
        ASSERT_TRUE(ptr != nullptr);
        ptr->migrate(getNthPosition(idx, i));
      }
    });

    local_set = theCollection()->getLocalIndices(proxy);
    cs_set = std::set<Index3D>{cs->getSet().begin(), cs->getSet().end()};
    EXPECT_EQ(local_set, cs_set);
    cs->phaseDone();
  }
}

TEST_F(TestCollectionChainsetTracking, test_home_chainset_tracking) {
  auto num_nodes = theContext()->getNumNodes();
  auto range = Index3D(2, static_cast<int>(num_nodes), 3);
  auto proxy = vt::makeCollection<MyCol>()
    .collective(true)
    .bounds(range)
    .bulkInsert()
    .wait();
  auto cs = std::make_unique<messaging::CollectionChainSet<Index3D>>(
    proxy, messaging::Home
  );

  theCollective()->barrier();

  auto local_set = theCollection()->getLocalIndices(proxy);
  auto cs_set = std::set<Index3D>{cs->getSet().begin(), cs->getSet().end()};
  EXPECT_EQ(local_set, cs_set);
  cs->phaseDone();

  theCollective()->barrier();

  auto getNthPosition = [&](Index3D idx, int n) -> NodeType {
    auto const home = theCollection()->getMappedNode(proxy, idx);
    auto num = vt::theContext()->getNumNodes();
    return (home + n) % num;
  };

  for (int i = 1; i < num_nodes + 1; i++) {
    runInEpochCollective("doMigrations", [&]{
      for (auto&& idx : theCollection()->getLocalIndices(proxy)) {
        auto ptr = proxy[idx].tryGetLocalPtr();
        ASSERT_TRUE(ptr != nullptr);
        ptr->migrate(getNthPosition(idx, i));
      }
    });

    // the set should always be the local set that we started with
    cs_set = std::set<Index3D>{cs->getSet().begin(), cs->getSet().end()};
    EXPECT_EQ(local_set, cs_set);
    cs->phaseDone();
  }
}


}}}} /* end namespace vt::tests::unit::termination */
