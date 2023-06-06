/*
//@HEADER
// *****************************************************************************
//
//                          test_lb_data_retention.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#include <vt/vrt/collection/balance/col_lb_data.h>
#include <vt/vrt/collection/balance/model/persistence_median_last_n.h>
#include <vt/vrt/collection/balance/model/load_model.h>
#include <vt/vrt/collection/balance/lb_invoke/lb_manager.h>
#include <vt/vrt/collection/types/type_aliases.h>
#include <vt/vrt/collection/manager.h>
#include <vt/phase/phase_manager.h>

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "test_helpers.h"

#include <memory>

namespace vt { namespace tests { namespace unit {

void validatePersistedPhases(std::vector<PhaseType> expected_phases) {
  #if vt_check_enabled(lblite)
    // Check maps size
    EXPECT_EQ(expected_phases.size(), theNodeLBData()->getLBData()->node_comm_.size());
    EXPECT_EQ(expected_phases.size(), theNodeLBData()->getLBData()->node_data_.size());
    EXPECT_EQ(expected_phases.size(), theNodeLBData()->getLBData()->node_subphase_comm_.size());
    EXPECT_EQ(expected_phases.size(), theNodeLBData()->getLBData()->user_defined_json_.size());
    EXPECT_EQ(expected_phases.size(), theNodeLBData()->getLBData()->user_defined_lb_info_.size());
    // Check if each phase is present
    for(auto&& phase : expected_phases) {
      EXPECT_EQ(1, theNodeLBData()->getLBData()->node_comm_.count(phase));
      EXPECT_EQ(1, theNodeLBData()->getLBData()->node_data_.count(phase));
      EXPECT_EQ(1, theNodeLBData()->getLBData()->node_subphase_comm_.count(phase));
      EXPECT_EQ(1, theNodeLBData()->getLBData()->user_defined_json_.count(phase));
      EXPECT_EQ(1, theNodeLBData()->getLBData()->user_defined_lb_info_.count(phase));
    }
  #else
    (void)expected_phases;
    EXPECT_EQ(0, theNodeLBData()->getLBData()->node_comm_.size());
    EXPECT_EQ(0, theNodeLBData()->getLBData()->node_data_.size());
    EXPECT_EQ(0, theNodeLBData()->getLBData()->node_subphase_comm_.size());
    EXPECT_EQ(0, theNodeLBData()->getLBData()->user_defined_json_.size());
    EXPECT_EQ(0, theNodeLBData()->getLBData()->user_defined_lb_info_.size());
  #endif
}

struct TestCol : vt::Collection<TestCol,vt::Index1D> {
  unsigned int prev_calls_ = thePhase()->getCurrentPhase();

  TestCol() {
    // Insert dummy lb info data
    valInsert("foo", 10, true, true);
  }

  unsigned int prevCalls() { return prev_calls_++; }

  static void colHandler(TestCol* col) {

    auto& lb_data = col->lb_data_;
    auto load_phase_count = lb_data.getLoadPhaseCount();
    auto comm_phase_count = lb_data.getCommPhaseCount();
    auto sp_load_phase_count = lb_data.getSubphaseLoadPhaseCount();
    auto sp_comm_phase_count = lb_data.getSubphaseCommPhaseCount();

    #if vt_check_enabled(lblite)
      auto phase = col->prevCalls();
      auto model = theLBManager()->getLoadModel();
      auto phases_needed = std::max(model->getNumPastPhasesNeeded(), theConfig()->vt_lb_data_retention);
      if (phase > phases_needed) {
        // updatePhase will have caused entries to be added for the
        // next phase already
        EXPECT_EQ(load_phase_count,    phases_needed + 1);
        EXPECT_EQ(sp_load_phase_count, phases_needed + 1);
        EXPECT_EQ(comm_phase_count,    phases_needed + 1);
        EXPECT_EQ(sp_comm_phase_count, phases_needed + 1);
      } else if (phase == 0) {
        EXPECT_EQ(load_phase_count,    phase);
        EXPECT_EQ(sp_load_phase_count, phase);
        EXPECT_EQ(comm_phase_count,    phase);
        EXPECT_EQ(sp_comm_phase_count, phase);
      } else {
        // updatePhase will have caused entries to be added for the
        // next phase already
        EXPECT_EQ(load_phase_count,    phase + 1);
        EXPECT_EQ(sp_load_phase_count, phase + 1);
        EXPECT_EQ(comm_phase_count,    phase + 1);
        EXPECT_EQ(sp_comm_phase_count, phase + 1);
      }
    #else
      EXPECT_EQ(load_phase_count,    0);
      EXPECT_EQ(sp_load_phase_count, 0);
      EXPECT_EQ(comm_phase_count,    0);
      EXPECT_EQ(sp_comm_phase_count, 0);
    #endif
  }

  static void expect7PhasesOfData(TestCol* col) {
    auto& lb_data = col->lb_data_;
    auto load_phase_count = lb_data.getLoadPhaseCount();
    auto comm_phase_count = lb_data.getCommPhaseCount();
    auto sp_load_phase_count = lb_data.getSubphaseLoadPhaseCount();
    auto sp_comm_phase_count = lb_data.getSubphaseCommPhaseCount();

    #if vt_check_enabled(lblite)
      auto phases_needed = 7;
      EXPECT_EQ(load_phase_count,    phases_needed);
      EXPECT_EQ(sp_load_phase_count, phases_needed);
      EXPECT_EQ(comm_phase_count,    phases_needed);
      EXPECT_EQ(sp_comm_phase_count, phases_needed);
    #else
      EXPECT_EQ(load_phase_count,    0);
      EXPECT_EQ(sp_load_phase_count, 0);
      EXPECT_EQ(comm_phase_count,    0);
      EXPECT_EQ(sp_comm_phase_count, 0);
    #endif
  }

  static void emptyColHandler(TestCol*) { }
};

static constexpr int32_t const num_elms = 16;

struct TestLBDataRetention : TestParallelHarness {
  virtual void SetUp() override {
    TestParallelHarness::SetUp();

    // We must have more or equal number of elements than nodes for this test to
    // work properly
    SET_MAX_NUM_NODES_CONSTRAINT(num_elms);
  }
};

using vt::vrt::collection::balance::PersistenceMedianLastN;

TEST_F(TestLBDataRetention, test_lbdata_retention_last1) {
  static constexpr int const num_phases = 5;

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct two collections
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(
      range, "test_lbstats_retention_last1"
    );
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
      proxy.broadcastCollective<TestCol::colHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }

  // Check amount of phase data in the node
  validatePersistedPhases({4});
}

TEST_F(TestLBDataRetention, test_lbdata_retention_last2) {
  static constexpr int const num_phases = 6;
  // Minimum retention lower than the amount of phases needed by model - will be ignored
  theConfig()->vt_lb_data_retention = 1;

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct two collections
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(
      range, "test_lbstats_retention_last2"
    );
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
      proxy.broadcastCollective<TestCol::colHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }

  // Check amount of phase data in the node
  validatePersistedPhases({4,5});
}

TEST_F(TestLBDataRetention, test_lbdata_retention_last4) {
  static constexpr int const num_phases = 8;
  // Minimum retention equal to the amount of phases needed by model
  theConfig()->vt_lb_data_retention = 4;

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct two collections
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(
      range, "test_lbstats_retention_last4"
    );
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
      proxy.broadcastCollective<TestCol::colHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }

  // Check amount of phase data in the node
  validatePersistedPhases({4,5,6,7});
}

TEST_F(TestLBDataRetention, test_lbdata_config_retention_higher) {
  // Minimum retention higher than the amount of phases needed by model
  theConfig()->vt_lb_data_retention = 6;

  // We must have more or equal number of elements than nodes for this test to
  // work properly
  EXPECT_GE(num_elms, vt::theContext()->getNumNodes());

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct two collections
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(
      range, "test_lbdata_config_retention_higher"
    );
  });

  // Get the base model, assert it's valid
  auto base = theLBManager()->getBaseLoadModel();
  EXPECT_NE(base, nullptr);

  // Create a new model
  auto persist = std::make_shared<PersistenceMedianLastN>(base, 4U);

  // Set the new model
  theLBManager()->setLoadModel(persist);

  for (uint32_t i=0; i<theConfig()->vt_lb_data_retention * 2; ++i) {
    runInEpochCollective([&]{
      // Do some work.
      proxy.broadcastCollective<TestCol::colHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }

  // Check amount of phase data in the node
  validatePersistedPhases({6,7,8,9,10,11});
}

TEST_F(TestLBDataRetention, test_lbdata_retention_model_switch_1) {
  static constexpr int const first_stage_num_phases = 11;
  theConfig()->vt_lb_data_retention = 0;

  // We must have more or equal number of elements than nodes for this test to
  // work properly
  EXPECT_GE(num_elms, vt::theContext()->getNumNodes());
  auto range = vt::Index1D(num_elms);
  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct two collections
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(
      range, "test_lbdata_retention_model_switch_1"
    );
  });

  // Get the base model, assert it's valid
  auto base = theLBManager()->getBaseLoadModel();
  EXPECT_NE(base, nullptr);

  // Create a new models
  auto model_10_phases = std::make_shared<PersistenceMedianLastN>(base, 10U);
  auto model_1_phase = std::make_shared<PersistenceMedianLastN>(base, 1U);

  // Set model which needs 10 phases of data
  theLBManager()->setLoadModel(model_10_phases);

  for (uint32_t i=0; i<first_stage_num_phases; ++i) {
    runInEpochCollective([&]{
      // Do some work.
      proxy.broadcastCollective<TestCol::colHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }

  // Set model which needs only 1 phase of data
  theLBManager()->setLoadModel(model_1_phase);

  // Check amount of phase data in the node
  validatePersistedPhases({10});

  for (uint32_t i=0; i<first_stage_num_phases; ++i) {
    runInEpochCollective([&]{
      // Do some work.
      proxy.broadcastCollective<TestCol::emptyColHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }

  // Check amount of phase data in the node
  validatePersistedPhases({21});
}

TEST_F(TestLBDataRetention, test_lbdata_retention_model_switch_2) {
  static constexpr int const first_stage_num_phases = 6;
  theConfig()->vt_lb_data_retention = 0;

  // We must have more or equal number of elements than nodes for this test to
  // work properly
  EXPECT_GE(num_elms, vt::theContext()->getNumNodes());
  auto range = vt::Index1D(num_elms);
  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct two collections
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(
      range, "test_lbdata_retention_model_switch_2"
    );
  });

  // Get the base model, assert it's valid
  auto base = theLBManager()->getBaseLoadModel();
  EXPECT_NE(base, nullptr);

  // Create and set model which needs 10 phases of data
  auto model_10_phases = std::make_shared<PersistenceMedianLastN>(base, 10U);
  theLBManager()->setLoadModel(model_10_phases);

  // Do only 6 phases of work
  for (uint32_t i=0; i<first_stage_num_phases; ++i) {
    runInEpochCollective([&]{
      // Do some work.
      proxy.broadcastCollective<TestCol::colHandler>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }

  // Check amount of phase data in the node
  validatePersistedPhases({0,1,2,3,4,5});

  // Set model which needs only 1 phase of data
  auto model_1_phase = std::make_shared<PersistenceMedianLastN>(base, 1U);
  theLBManager()->setLoadModel(model_1_phase);

  // Check amount of phase data in the node
  validatePersistedPhases({5});

  // Check that amount of the retained data in TestCol is not changed and still contains 7 phases of data
  for (uint32_t i=0; i<10; ++i) {
    runInEpochCollective([&]{
      // Do some work.
      proxy.broadcastCollective<TestCol::expect7PhasesOfData>();
    });
    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }

  // Check amount of phase data in the node
  validatePersistedPhases({15});
}

}}} // end namespace vt::tests::unit
