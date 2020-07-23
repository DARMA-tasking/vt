/*
//@HEADER
// *****************************************************************************
//
//                    test_collective_insertable.extended.cc
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

#include <vt/transport.h>
#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include <set>
#include <cmath>

namespace vt { namespace tests { namespace unit {

std::set<vt::Index1D> inserted_indices;

struct MyCol : vt::InsertableCollection<MyCol,Index1D> {
  MyCol() { inserted_indices.insert(getIndex()); }
};

using TestCollectiveInsertable = TestParallelHarness;

static constexpr int32_t const num_elms = 64;

vt::NodeType myMap(vt::Index1D* idx, vt::Index1D* range, vt::NodeType) {
  // block-like map
  auto num_nodes = vt::theContext()->getNumNodes();
  auto per_node = (NodeType)std::round((double)range->x() / num_nodes);
  return (idx->x() / per_node) % num_nodes;
}

TEST_F(TestCollectiveInsertable, test_collective_insertable_1) {

  vt::CollectionProxy<MyCol, vt::Index1D> proxy;

  auto range = vt::Index1D(num_elms);

  // Create the collection
  vt::runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<MyCol, &myMap>(range);
  });

  // No elements should be created at this point; this must be in a collective
  // epoch so it doesn't race with future insert messages
  vt::runInEpochCollective([&]{
    EXPECT_EQ(inserted_indices.size(), 0);
  });

  auto num_nodes = vt::theContext()->getNumNodes();
  auto this_node = vt::theContext()->getNode();

  vt::runInEpochCollective([&]{
    proxy.startInsertCollective();

    // insert even elements
    for (int i = 0; i < num_elms; i++) {
      // round-robin dispatch, some inserts will occur off-node due to block map
      if (i % 2 == 0 and i % num_nodes == this_node) {
        proxy[i].insert();
      }
    }

    proxy.finishInsertCollective();
  });

  // check that even elements, mapped here, exist!
  for (int i = 0; i < num_elms; i++) {
    vt::Index1D idx{i};
    if (i % 2 == 0 and myMap(&idx, &range, 0) == this_node) {
      EXPECT_NE(inserted_indices.find(idx), inserted_indices.end());
    }
  }

  vt::runInEpochCollective([&]{
    proxy.startInsertCollective();

    // insert odd elements
    for (int i = 0; i < num_elms; i++) {
      if (i % 2 != 0 and i % num_nodes == this_node) {
        proxy[i].insert();
      }
    }

    proxy.finishInsertCollective();
  });


  // check that all elements exist!
  for (int i = 0; i < num_elms; i++) {
    vt::Index1D idx{i};
    if (myMap(&idx, &range, 0) == this_node) {
      EXPECT_NE(inserted_indices.find(idx), inserted_indices.end());
    }
  }
}

}}} // end namespace vt::tests::unit
