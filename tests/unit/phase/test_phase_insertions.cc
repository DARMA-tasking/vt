/*
//@HEADER
// *****************************************************************************
//
//                           test_phase_insertions.cc
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
#include "data_message.h"
#include "test_helpers.h"

#include <vt/vrt/collection/manager.h>

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit {

static constexpr int const num_elms = 64;
static constexpr int const num_phases = 10;

static bool created_elm_0 = false;

struct MyCol : vt::Collection<MyCol,vt::Index1D> {
  MyCol() {
    fmt::print("{}: constructing element: idx={}\n", theContext()->getNode(), getIndex());
    if (getIndex() == vt::Index1D{0}) {
      EXPECT_FALSE(created_elm_0);
      created_elm_0 = true;
    }
  }

  explicit MyCol(checkpoint::SERIALIZE_CONSTRUCT_TAG) {}

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::InsertableCollection<MyCol,vt::Index1D>::serialize(s);
    s | val;
  }

  vt::PhaseType getPhase() { return this->getStats().getPhase(); }

  double val = 0.0;
};

using MyMsg = vt::CollectionMessage<MyCol>;

// A dummy kernel that does some work depending on the index
void colHandler(MyMsg*, MyCol* col) {
  fmt::print("running colHandler: idx={}, phase={}\n", col->getIndex(), col->getPhase());
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < col->getIndex().x() * 20; j++) {
      col->val += (i*29+j*2)-4;
    }
  }
}

static NodeType map_fn(Index1D* idx, Index1D* max_idx, NodeType nnodes) {
  return mapping::dense1DRoundRobinMap(idx, max_idx, nnodes);
}

using TestPhaseInsertions = TestParallelHarness;

TEST_F(TestPhaseInsertions, test_phase_insertions_1) {
  SET_MIN_NUM_NODES_CONSTRAINT(2);

  auto range = vt::Index1D(num_elms);

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  int insert_counter = range.x() / 2;

  auto proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .mapperFunc<map_fn>()
    .collective(true)
    .dynamicMembership(true)
    .wait();

  {
    auto token = proxy.beginInserting();

    for (int i = 0; i < range.x() / 2; i++) {
      if (i % num_nodes == this_node) {
        proxy[i].insert(token);
      }
    }

    proxy.finishInserting(std::move(token));
  }

  for (int phase = 0; phase < num_phases; phase++) {
    // Do some work.
    runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<MyMsg, colHandler>();
      }
    });

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    auto token = proxy.beginInserting();

    if (this_node == 0 and insert_counter < num_elms) {
      proxy[insert_counter].insert(token);
      insert_counter++;
    }

    // Try to re-insert an element that already exists to test for reinsertion
    // bugs
    if (this_node == 0 or this_node == 1) {
      proxy[0].insert(token, 1);
      proxy[0].insert(token, 0);
    }

    proxy.finishInserting(std::move(token));
  }
}

}}} // end namespace vt::tests::unit

#endif /*vt_check_enabled(lblite)*/
