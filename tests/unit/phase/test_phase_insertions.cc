/*
//@HEADER
// *****************************************************************************
//
//                           test_phase_insertions.cc
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

#include <vt/transport.h>

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit {

static constexpr int const num_elms = 64;
static constexpr int const num_phases = 10;

struct MyCol : vt::InsertableCollection<MyCol,vt::Index1D> {
  MyCol() {
    fmt::print("constructing element: idx={}\n", getIndex());
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::InsertableCollection<MyCol,vt::Index1D>::serialize(s);
    s | val;
  }

  double val = 0.0;
};

using MyMsg = vt::CollectionMessage<MyCol>;

// A dummy kernel that does some work depending on the index
void colHandler(MyMsg*, MyCol* col) {
  fmt::print("running colHandler: idx={}\n", col->getIndex());
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < col->getIndex().x() * 20; j++) {
      col->val += (i*29+j*2)-4;
    }
  }
}

using TestPhaseInsertions = TestParallelHarness;

TEST_F(TestPhaseInsertions, test_phase_insertions_1) {
  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<MyCol> proxy;

  auto this_node = theContext()->getNode();
  int insert_counter = range.x() / 2;

  // Construct a collection
  runInEpochCollective([&]{
    // For now, since insertable collections aren't allowed to be collectively
    // constructed, used a rooted construction with rooted broadcasts.
    if (this_node == 0) {
      proxy = vt::theCollection()->construct<MyCol>(range);
    }
  });

  // Insert every other element
  runInEpochCollective([&]{
    if (this_node == 0) {
      for (int i = 0; i < range.x() / 2; i++) {
        proxy[i].insert();
      }
    }
  });

  for (int phase = 0; phase < num_phases; phase++) {
    // Do some work.
    runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<MyMsg, colHandler>();
      }
    });

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();

    // Insert the next element
    runInEpochCollective([&]{
      if (this_node == 0 and insert_counter < num_elms) {
        proxy[insert_counter].insert();
        insert_counter++;
      }
    });
  }
}

}}} // end namespace vt::tests::unit

#endif /*vt_check_enabled(lblite)*/
