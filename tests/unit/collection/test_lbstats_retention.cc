/*
//@HEADER
// *****************************************************************************
//
//                         test_lbstats_retention.cc
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
#include <vt/vrt/collection/balance/elm_stats.h>
#include <vt/vrt/collection/balance/model/persistence_median_last_n.h>
#include <vt/vrt/collection/balance/model/load_model.h>

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit {

template <typename ColT>
struct MyMsg : vt::CollectionMessage<ColT> { };

struct TestCol : vt::Collection<TestCol,vt::Index1D> {
  unsigned int prev_calls_ = 0;

  unsigned int prevCalls() {
    fmt::print(
      "{}: {}: prev_calls={}\n",
      theContext()->getNode(), getIndex(), prev_calls_
    );
    return prev_calls_++;
  }

  static void colHandler(MyMsg<TestCol>* msg, TestCol* col) {
    auto phase = col->prevCalls();
    auto model = theLBManager()->getLoadModel();
    auto phases_needed = model->getNumPastPhasesNeeded();

    auto& stats = col->stats_;
    auto load_phase_count = stats.getLoadPhaseCount();
    auto comm_phase_count = stats.getCommPhaseCount();
    auto sp_load_phase_count = stats.getSubphaseLoadPhaseCount();
    auto sp_comm_phase_count = stats.getSubphaseCommPhaseCount();

    if (phase > phases_needed) {
      // syncNextPhase will have caused timings to be added for the
      // next phase already
      EXPECT_EQ(load_phase_count, phases_needed + 1);
      EXPECT_EQ(sp_load_phase_count, phases_needed + 1);

      EXPECT_EQ(comm_phase_count, phases_needed);
      EXPECT_EQ(sp_comm_phase_count, phases_needed);
    } else if (phase == 0) {
      EXPECT_EQ(load_phase_count, phase);
      EXPECT_EQ(sp_load_phase_count, phase);

      EXPECT_EQ(comm_phase_count, phase);
      EXPECT_EQ(sp_comm_phase_count, phase);
    } else {
      // syncNextPhase will have caused timings to be added for the
      // next phase already
      EXPECT_EQ(load_phase_count, phase + 1);
      EXPECT_EQ(sp_load_phase_count, phase + 1);

      EXPECT_EQ(comm_phase_count, phase);
      EXPECT_EQ(sp_comm_phase_count, phase);
    }
  }
};

using TestLBstatsRetention = TestParallelHarness;

using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::PersistenceMedianLastN;

static constexpr int32_t const num_elms = 16;

TEST_F(TestLBstatsRetention, test_lbstats_retention_last1) {
  static constexpr int const num_phases = 5;

  // We must have more or equal number of elements than nodes for this test to
  // work properly
  EXPECT_GE(num_elms, vt::theContext()->getNumNodes());

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct two collections
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(range);
  });

  // Get the base model, assert it's valid
  auto base = theLBManager()->getBaseLoadModel();
  EXPECT_NE(base, nullptr);

  // Create a new model
  auto persist = std::make_shared<PersistenceMedianLastN>(base, 1U);

  // Set the new model
  theLBManager()->setLoadModel(persist);

  for (int i=0; i<num_phases; ++i) {
    runInEpochCollective([&]{
      // Do some work.
      proxy.broadcastCollective<MyMsg<TestCol>, TestCol::colHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }
}

TEST_F(TestLBstatsRetention, test_lbstats_retention_last2) {
  static constexpr int const num_phases = 6;

  // We must have more or equal number of elements than nodes for this test to
  // work properly
  EXPECT_GE(num_elms, vt::theContext()->getNumNodes());

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct two collections
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(range);
  });

  // Get the base model, assert it's valid
  auto base = theLBManager()->getBaseLoadModel();
  EXPECT_NE(base, nullptr);

  // Create a new model
  auto persist = std::make_shared<PersistenceMedianLastN>(base, 2U);

  // Set the new model
  theLBManager()->setLoadModel(persist);

  for (int i=0; i<num_phases; ++i) {
    runInEpochCollective([&]{
      // Do some work.
      proxy.broadcastCollective<MyMsg<TestCol>, TestCol::colHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }
}

TEST_F(TestLBstatsRetention, test_lbstats_retention_last4) {
  static constexpr int const num_phases = 8;

  // We must have more or equal number of elements than nodes for this test to
  // work properly
  EXPECT_GE(num_elms, vt::theContext()->getNumNodes());

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct two collections
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(range);
  });

  // Get the base model, assert it's valid
  auto base = theLBManager()->getBaseLoadModel();
  EXPECT_NE(base, nullptr);

  // Create a new model
  auto persist = std::make_shared<PersistenceMedianLastN>(base, 4U);

  // Set the new model
  theLBManager()->setLoadModel(persist);

  for (int i=0; i<num_phases; ++i) {
    runInEpochCollective([&]{
      // Do some work.
      proxy.broadcastCollective<MyMsg<TestCol>, TestCol::colHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }
}

}}} // end namespace vt::tests::unit
