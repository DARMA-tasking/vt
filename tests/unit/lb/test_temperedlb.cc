#include <vt/transport.h>
#include <vt/vrt/collection/balance/read_lb.h>
#include <vt/vrt/collection/balance/workload_replay.h>

#include "test_helpers.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace lb {

#if vt_check_enabled(lblite)

using TestTemperedLB = TestParallelHarness;

std::string writeTemperedLBConfig(std::string transfer_strategy,
                                  bool mem_constraints,
                                  double delta = 0.0,
                                  double beta = 0.0,
                                  double gamma = 0.0) {
    int this_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &this_rank);
    auto config_file = getUniqueFilename();
    if (this_rank == 0) {
        std::ofstream cfg_file_{config_file.c_str(), std::ofstream::out | std::ofstream::trunc};
        cfg_file_ << "0 TemperedLB transfer=" << transfer_strategy <<
                     " alpha=1.0" <<
                     " beta=" << beta <<
                     " gamma=" << gamma <<
                     " delta=" << delta;
        if (transfer_strategy == "SwapClusters") {
            if (mem_constraints) {
                cfg_file_ << " memory_threshold=20.0";
            } else {
                cfg_file_ << " memory_threshold=1e8";
            }
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
        initial_phase, phases_to_run, phase_mod);

    // Get information for the last phase (this problem only has one)
    auto phase_info = theLBManager()->getPhaseInfo();

    // Assert that temperedLB found the correct imbalance
    EXPECT_EQ(phase_info->imb_load_post_lb, expected_imb);
}

TEST_F(TestTemperedLB, test_load_only) {
    SET_NUM_NODES_CONSTRAINT(4);
    auto cfg = writeTemperedLBConfig("SwapClusters", false);
    runTemperedLBTest(cfg);
}

TEST_F(TestTemperedLB, test_load_and_memory_swapclusters) {
    SET_NUM_NODES_CONSTRAINT(4);
    auto cfg = writeTemperedLBConfig("SwapClusters", true);
    runTemperedLBTest(cfg);
}

TEST_F(TestTemperedLB, test_load_no_memory_delta_10) {
    SET_NUM_NODES_CONSTRAINT(4);
    auto cfg = writeTemperedLBConfig("SwapClusters", false, 1.0);
    runTemperedLBTest(cfg);
}

TEST_F(TestTemperedLB, test_load_no_memory_delta_01) {
    SET_NUM_NODES_CONSTRAINT(4);
    auto cfg = writeTemperedLBConfig("SwapClusters", false, 0.1);
    runTemperedLBTest(cfg);
}

TEST_F(TestTemperedLB, test_load_memory_homing_swapclusters) {
    SET_NUM_NODES_CONSTRAINT(4);
    auto cfg = writeTemperedLBConfig("SwapClusters", true, 0.1);
    runTemperedLBTest(cfg);
}

TEST_F(TestTemperedLB, test_load_memory_homing_comms) {
    SET_NUM_NODES_CONSTRAINT(4);
    auto cfg = writeTemperedLBConfig("SwapClusters", true, 0.1, 1.0);
    double expected_imbalance = 0.0; // placeholder for value from MILP
    runTemperedLBTest(cfg, expected_imbalance);
}

#endif

}}}} /* end namespace vt::tests::unit::lb */
