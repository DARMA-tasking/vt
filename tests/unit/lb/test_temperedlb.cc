/*
//@HEADER
// *****************************************************************************
//
//                              test_temperedlb.cc
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
#include <vt/transport.h>
#include <vt/vrt/collection/balance/read_lb.h>
#include <vt/vrt/collection/balance/workload_replay.h>

#include "test_helpers.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace lb {

#if vt_check_enabled(lblite)

using TestTemperedLB = TestParallelHarness;

std::string writeTemperedLBConfig(
  std::string transfer_strategy, double memory_threshold, double alpha = 1.0,
  double beta = 0.0, double gamma = 0.0, double delta = 0.0
) {
  auto const this_node = theContext()->getNode();
  auto config_file = getUniqueFilename();
  if (this_node == 0) {
    std::ofstream cfg_file_{config_file.c_str(), std::ofstream::out | std::ofstream::trunc};
    cfg_file_ << "0 TemperedLB iters=10 trials=3 transfer=" << transfer_strategy <<
      " alpha=" << alpha <<
      " beta=" << beta <<
      " gamma=" << gamma <<
      " delta=" << delta;
    if (transfer_strategy == "SwapClusters") {
        cfg_file_ << " memory_threshold=" << memory_threshold;
    }
    cfg_file_.close();
  }
  return config_file;
}

void runTemperedLBTest(std::string config_file, double expected_imb = 0.0) {
  // Clear the LB config
  vrt::collection::balance::ReadLBConfig::clear();

  // Set configuration
  theConfig()->vt_lb = true;
  theConfig()->vt_lb_data_in = true;
  theConfig()->vt_lb_file_name = config_file;
  theConfig()->vt_lb_data_file_in="synthetic-dataset-blocks.%p.json";
  theConfig()->vt_lb_data_dir_in="synthetic-blocks-data";

  // Replay load balancing
  int initial_phase = 0;
  int phases_to_run = 1;
  int phase_mod = 0;
  vt::vrt::collection::balance::replay::replayWorkloads(
    initial_phase, phases_to_run, phase_mod
  );

  // Get information for the last phase
  auto phase_info = theLBManager()->getPhaseInfo();

  // Assert that temperedLB found the correct imbalance
  EXPECT_EQ(phase_info->imb_load_post_lb, expected_imb);
}

// The following tests use expected values found by the MILP

TEST_F(TestTemperedLB, test_load_only_original) {
  SET_NUM_NODES_CONSTRAINT(4);
  auto cfg = writeTemperedLBConfig("Original", 1e8);
  runTemperedLBTest(cfg);
}

TEST_F(TestTemperedLB, test_load_only_swapclusters) {
  SET_NUM_NODES_CONSTRAINT(4);
  auto cfg = writeTemperedLBConfig("SwapClusters", 1e8);
  runTemperedLBTest(cfg, 0.25);
}

TEST_F(TestTemperedLB, test_load_and_memory_swapclusters) {
  SET_NUM_NODES_CONSTRAINT(4);
  auto cfg = writeTemperedLBConfig("SwapClusters", 20);
  runTemperedLBTest(cfg, 0.25);
}

TEST_F(TestTemperedLB, test_load_no_memory_delta_10) {
  SET_NUM_NODES_CONSTRAINT(4);
  auto cfg = writeTemperedLBConfig("SwapClusters", 1e8, 1, 0, 0, 1);
  runTemperedLBTest(cfg, 1.0);
}

TEST_F(TestTemperedLB, test_load_no_memory_delta_01) {
  SET_NUM_NODES_CONSTRAINT(4);
  auto cfg = writeTemperedLBConfig("SwapClusters", 1e8, 1, 0, 0, 0.1);
  runTemperedLBTest(cfg, 0.25);
}

TEST_F(TestTemperedLB, test_load_memory_delta_01) {
  SET_NUM_NODES_CONSTRAINT(4);
  auto cfg = writeTemperedLBConfig("SwapClusters", 20, 1, 0, 0, 0.1);
  runTemperedLBTest(cfg, 0.25);
}

TEST_F(TestTemperedLB, test_load_no_memory_delta_03) {
  SET_NUM_NODES_CONSTRAINT(4);
  auto cfg = writeTemperedLBConfig("SwapClusters", 1e8, 1, 0, 0, 0.3);
  runTemperedLBTest(cfg, 1.0);
}

TEST_F(TestTemperedLB, test_load_memory_delta_03) {
  SET_NUM_NODES_CONSTRAINT(4);
  auto cfg = writeTemperedLBConfig("SwapClusters", 20, 1, 0, 0, 0.3);
  runTemperedLBTest(cfg, 1.0);
}

#endif

}}}} /* end namespace vt::tests::unit::lb */
