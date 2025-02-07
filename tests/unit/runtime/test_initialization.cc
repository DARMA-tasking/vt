/*
//@HEADER
// *****************************************************************************
//
//                            test_initialization.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "test_helpers.h"

#include <vt/collective/startup.h>
#include <vt/vrt/collection/balance/lb_data_restart_reader.h>
#include <vt/utils/json/json_appender.h>
#include <vt/elm/elm_id_bits.h>

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <filesystem>

namespace vt { namespace tests { namespace unit {

struct TestInitialization : TestParallelHarness { };

TEST_F(TestInitialization, test_initialize_with_args) {
  MPI_Comm comm = MPI_COMM_WORLD;

  static char prog_name[]{"vt_program"};
  static char cli_argument[]{"--cli_argument=100"};
  static char vt_no_terminate[]{"--vt_no_terminate"};

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(cli_argument);
  custom_args.emplace_back(vt_no_terminate);
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  EXPECT_EQ(custom_argc, 3);

  vt::initialize(custom_argc, custom_argv, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_no_terminate, true);

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

TEST_F(TestInitialization, test_initialize_with_appconfig) {
  MPI_Comm comm = MPI_COMM_WORLD;

  static char prog_name[]{"vt_program"};
  static char cli_argument[]{"--cli_argument=100"};

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(cli_argument);
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  EXPECT_EQ(custom_argc, 2);

  arguments::AppConfig appConfig{};
  appConfig.vt_epoch_graph_on_hang = false;
  appConfig.vt_lb_name = "RotateLB";
  appConfig.vt_lb_data = true;

  vt::initialize(custom_argc, custom_argv, &comm, &appConfig);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_epoch_graph_on_hang, false);
  EXPECT_EQ(theConfig()->vt_lb_name, "RotateLB");
  EXPECT_EQ(theConfig()->vt_lb_data, true);

  // vt_no_detect_hang wasn't set, should be default
  EXPECT_EQ(theConfig()->vt_no_detect_hang, false);

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

TEST_F(TestInitialization, test_initialize_with_args_and_appconfig) {
  MPI_Comm comm = MPI_COMM_WORLD;

  static char prog_name[]{"vt_program"};
  static char cli_argument[]{"--cli_argument=100"};
  static char vt_no_terminate[]{"--vt_no_terminate"};
  static char vt_no_detect_hang[]{"--vt_no_detect_hang"};

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(cli_argument);
  custom_args.emplace_back(vt_no_terminate);
  custom_args.emplace_back(vt_no_detect_hang);
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  EXPECT_EQ(custom_argc, 4);

  arguments::AppConfig appConfig{};
  appConfig.vt_color = false;
  appConfig.vt_epoch_graph_on_hang = false;
  appConfig.vt_lb_name = "RotateLB";
  appConfig.vt_lb_data = true;
  appConfig.vt_no_detect_hang = false;

  vt::initialize(custom_argc, custom_argv, &comm, &appConfig);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_color, false);
  EXPECT_EQ(theConfig()->vt_epoch_graph_on_hang, false);
  EXPECT_EQ(theConfig()->vt_lb_name, "RotateLB");
  EXPECT_EQ(theConfig()->vt_lb_data, true);
  EXPECT_EQ(theConfig()->vt_no_terminate, true);
  // CLI args should overwrite hardcoded appConfig
  EXPECT_EQ(theConfig()->vt_no_detect_hang, true);

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

TEST_F(TestInitialization, test_initialize_with_toml_file_and_args) {
  MPI_Comm comm = MPI_COMM_WORLD;

  static char prog_name[]{"vt_program"};
  static char cli_argument[]{"--cli_argument=100"};
  static char vt_no_terminate[]{"--vt_no_terminate"};
  static char vt_lb_name[]{"--vt_lb_name=RotateLB"};

  std::string config_file(getUniqueFilenameWithRanks(".toml"));
  std::string config_flag("--vt_input_config=");
  std::string vt_input_config = config_flag + config_file;

  std::vector<char *> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(cli_argument);
  custom_args.emplace_back(vt_no_terminate);
  custom_args.emplace_back(strdup(vt_input_config.c_str()));
  custom_args.emplace_back(vt_lb_name);
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char **custom_argv = custom_args.data();

  EXPECT_EQ(custom_argc, 5);

  int this_rank;
  MPI_Comm_rank(comm, &this_rank);
  if (this_rank == 0) {
    std::ofstream cfg_file_{config_file.c_str(), std::ofstream::out | std::ofstream::trunc};
    cfg_file_ << "vt_lb_name = RandomLB\n";
    cfg_file_.close();
  }
  MPI_Barrier(comm);

  vt::initialize(custom_argc, custom_argv, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_no_terminate, true);
  EXPECT_EQ(theConfig()->vt_lb_name, "RotateLB");

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

TEST_F(TestInitialization, test_initialize_with_toml_file_args_and_appconfig) {
  MPI_Comm comm = MPI_COMM_WORLD;

  static char prog_name[]{"vt_program"};
  static char cli_argument[]{"--cli_argument=100"};
  static char vt_no_terminate[]{"--vt_no_terminate"};
  static char vt_lb_name[]{"--vt_lb_name=RotateLB"};

  std::string config_file(getUniqueFilenameWithRanks(".toml"));
  std::string config_flag("--vt_input_config=");
  std::string vt_input_config = config_flag + config_file;

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(cli_argument);
  custom_args.emplace_back(vt_no_terminate);
  custom_args.emplace_back(strdup(vt_input_config.c_str()));
  custom_args.emplace_back(vt_lb_name);
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  EXPECT_EQ(custom_argc, 5);

  arguments::AppConfig appConfig{};
  appConfig.vt_lb_name = "GreedyLB";

  int this_rank;
  MPI_Comm_rank(comm, &this_rank);
  if (this_rank == 0) {
    std::ofstream cfg_file_{config_file.c_str(), std::ofstream::out | std::ofstream::trunc};
    cfg_file_ << "vt_lb_name = RandomLB\n";
    cfg_file_.close();
  }
  MPI_Barrier(comm);

  vt::initialize(custom_argc, custom_argv, &comm, &appConfig);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_no_terminate, true);
  EXPECT_EQ(theConfig()->vt_lb_name, "RotateLB");

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

TEST_F(TestInitialization, test_initialize_with_yaml) {
  MPI_Comm comm = MPI_COMM_WORLD;

  static char prog_name[]{"vt_program"};

  std::string config_file(getUniqueFilenameWithRanks(".yaml"));
  std::string config_flag("--vt_input_config_yaml=");
  std::string vt_input_config = config_flag + config_file;

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(strdup(vt_input_config.c_str()));
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  EXPECT_EQ(custom_argc, 2);

  int this_rank;
  MPI_Comm_rank(comm, &this_rank);
  if (this_rank == 0) {
    std::ofstream cfg_file_{config_file.c_str(), std::ofstream::out | std::ofstream::trunc};
    cfg_file_ << R"(
    Output Control:
      Color: False
      Quiet: True
    Signal Handling:
      Disable SIGINT: True
      Disable SIGSEGV: True
      Disable SIGBUS: True
      Disable Terminate Signal: True
    Memory Usage Reporting:
      Print Memory Each Phase: True
      Print Memory On Node: '1'
      Allow Memory Report With ps: True
    Tracing Configuration:
      Enabled: False
    Debug Print Configuration:
      Level: normal
      Enable:
        - gen
        - term
        - pool
        - group
    Load Balancing:
      Enabled: False
      LB Data Output:
        Enabled: False
      LB Data Input:
        Enabled: False
      LB Statistics:
        Enabled: False
    Diagnostics:
      Enabled: True
      Enable Print Summary: True
    Termination:
      No Detect Hangs: True
      Terse Epoch Graph Output: True
    Launch:
      Pause: False
    User Options:
      Test bool: True
      Test int: 45
      Test string: test_string
    Scheduler Configuration:
      Num Progress Times: 3
    Runtime:
      Throw on Abort: True
    Configuration File:
      Enable Output Config: True
      File: test_config.yaml
    Visualization:
      Enabled: False
    )";
    cfg_file_.close();
  }

  MPI_Barrier(comm);

  vt::initialize(custom_argc, custom_argv, &comm);

  // TEST THAT THE CONFIGURATION FILE WAS READ IN CORRECTLY
  EXPECT_EQ(theConfig()->prog_name, "vt_program");

  // Output Control
  EXPECT_EQ(theConfig()->vt_color, false);
#ifdef VT_NO_COLOR_ENABLED
  EXPECT_EQ(theConfig()->vt_no_color, true);
#else
  EXPECT_EQ(theConfig()->vt_no_color, false);
#endif
  EXPECT_EQ(theConfig()->vt_quiet, true);

  // Signal Handling
  EXPECT_EQ(theConfig()->vt_no_sigint, true);
  EXPECT_EQ(theConfig()->vt_no_sigsegv, true);
  EXPECT_EQ(theConfig()->vt_no_sigbus, true);
  EXPECT_EQ(theConfig()->vt_no_terminate, true);

  // Memory Usage Reporting
  EXPECT_EQ(theConfig()->vt_print_memory_each_phase, true);
  EXPECT_EQ(theConfig()->vt_print_memory_node, "1");
  EXPECT_EQ(theConfig()->vt_allow_memory_report_with_ps, true);
  EXPECT_EQ(theConfig()->vt_print_memory_threshold, "1 GiB");
  EXPECT_EQ(theConfig()->vt_print_memory_sched_poll, 100);
  EXPECT_EQ(theConfig()->vt_print_memory_footprint, false);

  // Dump Stack Backtrace
  EXPECT_EQ(theConfig()->vt_no_warn_stack, false);
  EXPECT_EQ(theConfig()->vt_no_assert_stack, false);
  EXPECT_EQ(theConfig()->vt_no_abort_stack, false);
  EXPECT_EQ(theConfig()->vt_no_stack, false);
  EXPECT_EQ(theConfig()->vt_stack_file, "");
  EXPECT_EQ(theConfig()->vt_stack_dir, "");
  EXPECT_EQ(theConfig()->vt_stack_mod, 0);

  // Tracing Configuration
  EXPECT_EQ(theConfig()->vt_trace, false);
  EXPECT_EQ(theConfig()->vt_trace_mpi, false);
  EXPECT_EQ(theConfig()->vt_trace_pmpi, false);
  EXPECT_EQ(theConfig()->vt_trace_mod, 0);
  EXPECT_EQ(theConfig()->vt_trace_flush_size, 0);
  EXPECT_EQ(theConfig()->vt_trace_gzip_finish_flush, false);
  EXPECT_EQ(theConfig()->vt_trace_sys_all, false);
  EXPECT_EQ(theConfig()->vt_trace_sys_term, false);
  EXPECT_EQ(theConfig()->vt_trace_sys_location, false);
  EXPECT_EQ(theConfig()->vt_trace_sys_collection, false);
  EXPECT_EQ(theConfig()->vt_trace_sys_serial_msg, false);
  EXPECT_EQ(theConfig()->vt_trace_spec, false);
  EXPECT_EQ(theConfig()->vt_trace_spec_file, "");
  EXPECT_EQ(theConfig()->vt_trace_memory_usage, false);
  EXPECT_EQ(theConfig()->vt_trace_event_polling, false);
  EXPECT_EQ(theConfig()->vt_trace_irecv_polling, false);


  EXPECT_EQ(theConfig()->vt_debug_level, "normal");
  EXPECT_EQ(theConfig()->vt_debug_all, false);
  EXPECT_EQ(theConfig()->vt_debug_none, false);
  EXPECT_EQ(theConfig()->vt_debug_print_flush, false);
  EXPECT_EQ(theConfig()->vt_debug_gen, true);
  EXPECT_EQ(theConfig()->vt_debug_runtime, false);
  EXPECT_EQ(theConfig()->vt_debug_active, false);
  EXPECT_EQ(theConfig()->vt_debug_term, true);
  EXPECT_EQ(theConfig()->vt_debug_termds, false);
  EXPECT_EQ(theConfig()->vt_debug_barrier, false);
  EXPECT_EQ(theConfig()->vt_debug_event, false);
  EXPECT_EQ(theConfig()->vt_debug_pipe, false);
  EXPECT_EQ(theConfig()->vt_debug_pool, true);
  EXPECT_EQ(theConfig()->vt_debug_reduce, false);
  EXPECT_EQ(theConfig()->vt_debug_rdma, false);
  EXPECT_EQ(theConfig()->vt_debug_rdma_channel, false);
  EXPECT_EQ(theConfig()->vt_debug_rdma_state, false);
  EXPECT_EQ(theConfig()->vt_debug_handler, false);
  EXPECT_EQ(theConfig()->vt_debug_hierlb, false);
  EXPECT_EQ(theConfig()->vt_debug_temperedlb, false);
  EXPECT_EQ(theConfig()->vt_debug_temperedwmin, false);
  EXPECT_EQ(theConfig()->vt_debug_scatter, false);
  EXPECT_EQ(theConfig()->vt_debug_serial_msg, false);
  EXPECT_EQ(theConfig()->vt_debug_trace, false);
  EXPECT_EQ(theConfig()->vt_debug_location, false);
  EXPECT_EQ(theConfig()->vt_debug_lb, false);
  EXPECT_EQ(theConfig()->vt_debug_vrt, false);
  EXPECT_EQ(theConfig()->vt_debug_vrt_coll, false);
  EXPECT_EQ(theConfig()->vt_debug_worker, false);
  EXPECT_EQ(theConfig()->vt_debug_group, true);
  EXPECT_EQ(theConfig()->vt_debug_broadcast, false);
  EXPECT_EQ(theConfig()->vt_debug_objgroup, false);
  EXPECT_EQ(theConfig()->vt_debug_phase, false);
  EXPECT_EQ(theConfig()->vt_debug_context, false);
  EXPECT_EQ(theConfig()->vt_debug_epoch, false);

  // Load Balancing
  EXPECT_EQ(theConfig()->vt_lb, false);
  EXPECT_EQ(theConfig()->vt_lb_quiet, false);
  EXPECT_EQ(theConfig()->vt_lb_file_name, "");
  EXPECT_EQ(theConfig()->vt_lb_show_config, false);
  EXPECT_EQ(theConfig()->vt_lb_name, "NoLB");
  EXPECT_EQ(theConfig()->vt_lb_args, "");
  EXPECT_EQ(theConfig()->vt_lb_interval, 1);
  EXPECT_EQ(theConfig()->vt_lb_keep_last_elm, false);
  EXPECT_EQ(theConfig()->vt_lb_data, false);
  EXPECT_EQ(theConfig()->vt_lb_data_dir, "vt_lb_data");
  EXPECT_EQ(theConfig()->vt_lb_data_file, "data.%p.json");
  EXPECT_EQ(theConfig()->vt_lb_data_in, false);
  EXPECT_EQ(theConfig()->vt_lb_data_compress, true);
  EXPECT_EQ(theConfig()->vt_lb_data_dir_in, "vt_lb_data_in");
  EXPECT_EQ(theConfig()->vt_lb_data_file_in, "data.%p.json");
  EXPECT_EQ(theConfig()->vt_lb_statistics, false);
  EXPECT_EQ(theConfig()->vt_lb_statistics_compress, true);
  EXPECT_EQ(theConfig()->vt_lb_statistics_file, "vt_lb_statistics.%t.json");
  EXPECT_EQ(theConfig()->vt_lb_statistics_dir, "");
  EXPECT_EQ(theConfig()->vt_lb_self_migration, false);
  EXPECT_EQ(theConfig()->vt_lb_spec, false);
  EXPECT_EQ(theConfig()->vt_lb_spec_file, "");

  // Diagnostics
  EXPECT_EQ(theConfig()->vt_diag_enable, true);
  EXPECT_EQ(theConfig()->vt_diag_print_summary, true);
  EXPECT_EQ(theConfig()->vt_diag_summary_file, "vtdiag.txt");
  EXPECT_EQ(theConfig()->vt_diag_summary_csv_file, "");
  EXPECT_EQ(theConfig()->vt_diag_csv_base_units, false);

  // Termination
  EXPECT_EQ(theConfig()->vt_no_detect_hang, true);
  EXPECT_EQ(theConfig()->vt_term_rooted_use_ds, false);
  EXPECT_EQ(theConfig()->vt_term_rooted_use_wave, false);
  EXPECT_EQ(theConfig()->vt_epoch_graph_on_hang, true);
  EXPECT_EQ(theConfig()->vt_epoch_graph_terse, true);
  EXPECT_EQ(theConfig()->vt_print_no_progress, true);
  EXPECT_EQ(theConfig()->vt_hang_freq, 1024);

  // Debugging/Launch
  EXPECT_EQ(theConfig()->vt_pause, false);

  // User Options
  EXPECT_EQ(theConfig()->vt_user_1, true);
  EXPECT_EQ(theConfig()->vt_user_2, false);
  EXPECT_EQ(theConfig()->vt_user_3, false);
  EXPECT_EQ(theConfig()->vt_user_int_1, 45);
  EXPECT_EQ(theConfig()->vt_user_int_2, 0);
  EXPECT_EQ(theConfig()->vt_user_int_3, 0);
  EXPECT_EQ(theConfig()->vt_user_str_1, "test_string");
  EXPECT_EQ(theConfig()->vt_user_str_2, "");
  EXPECT_EQ(theConfig()->vt_user_str_3, "");

  // Scheduler Configuration
  EXPECT_EQ(theConfig()->vt_sched_num_progress, 3);
  EXPECT_EQ(theConfig()->vt_sched_progress_han, 0);
  EXPECT_EQ(theConfig()->vt_sched_progress_sec, 0);

  // Runtime
  EXPECT_EQ(theConfig()->vt_max_mpi_send_size, 1ull << 30);
  EXPECT_EQ(theConfig()->vt_no_assert_fail, false);
  EXPECT_EQ(theConfig()->vt_throw_on_abort, true);

  // Visualization
  EXPECT_EQ(theConfig()->vt_tv, false);
  EXPECT_EQ(theConfig()->vt_tv_config_file, "");

  // TEST THAT THE CONFIGURATION FILE WAS WRITTEN OUT CORRECTLY
  YAML::Node input_config = YAML::LoadFile(config_file);
  YAML::Node output_config = YAML::Load(theConfig()->vt_output_config_str);
  assertYamlNodesHaveIdenticalEntries(input_config, output_config);
}

void prepareLBDataFiles(const std::string file_name_without_ext) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;
  using ElementIDStruct = vt::vrt::collection::balance::ElementIDStruct;
  using LoadSummary = vt::vrt::collection::balance::LoadSummary;

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  auto const next_node = (this_node + 1) % num_nodes;
  auto const prev_node = this_node - 1 >= 0 ? this_node - 1 : num_nodes - 1;

  std::unordered_map<PhaseType, std::vector<ElementIDStruct>> ids;
  int len = 2;
  PhaseType num_phases = 7;
  for (int i = 0; i < len; i++) {
    auto id = vt::elm::ElmIDBits::createCollectionImpl(true, i+1, this_node, this_node);
    id.curr_node = this_node;
    ids[0].push_back(id);
    id.curr_node = next_node;
    ids[3].push_back(id);
    id.curr_node = prev_node;
    ids[6].push_back(id);
  }

  for (int i = 0; i < len; i++) {
    auto pid = vt::elm::ElmIDBits::createCollectionImpl(true, i+1, prev_node, this_node);
    auto nid = vt::elm::ElmIDBits::createCollectionImpl(true, i+1, next_node, this_node);
    ids[1].push_back(pid);
    ids[2].push_back(pid);
    ids[4].push_back(nid);
    ids[5].push_back(nid);
  }

  LBDataHolder dh;
  dh.resizeHistory(num_phases);
  for (PhaseType i = 0; i < num_phases; i++) {
    for (auto&& elm : ids[i]) {
      dh.node_data_[i][elm] = LoadSummary{3};
      std::vector<uint64_t> arr = {1};
      VirtualProxyType proxy = 7;
      dh.node_idx_[elm] = std::make_tuple(proxy, arr);
    }
  }

  using JSONAppender = util::json::Appender<std::stringstream>;
  std::stringstream stream{std::ios_base::out | std::ios_base::in};
  nlohmann::json metadata;
  metadata["type"] = "LBDatafile";
  auto w = std::make_unique<JSONAppender>(
    "phases", metadata, std::move(stream), true
  );
  for (PhaseType i = 0; i < num_phases; i++) {
    auto j = dh.toJson(i);
    w->addElm(*j);
  }
  stream = w->finish();

  // save to file
  std::string file_name = file_name_without_ext + "." + std::to_string(this_node) + ".json";
  std::filesystem::path file_path = std::filesystem::current_path() / file_name;
  std::ofstream out(file_path);
  out << stream.str();
  out.close();
}

TEST_F(TestInitialization, test_initialize_without_restart_reader) {
  MPI_Comm comm = MPI_COMM_WORLD;

  static char prog_name[]{"vt_program"};

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  vt::initialize(custom_argc, custom_argv, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_lb_name, "NoLB");
  EXPECT_EQ(theConfig()->vt_lb_data_in, false);
  EXPECT_EQ(theConfig()->vt_lb_file_name, "");
  EXPECT_TRUE(theLBDataReader() == nullptr);
}

#if vt_feature_cmake_lblite
TEST_F(TestInitialization, test_initialize_with_lb_data_in) {
  MPI_Comm comm = MPI_COMM_WORLD;

  // Prepare data files
  auto prefix = getUniqueFilenameWithRanks();
  prepareLBDataFiles(prefix);

  static char prog_name[]{"vt_program"};
  static char data_in[]{"--vt_lb_data_in"};
  std::string data_file_dir = "--vt_lb_data_dir_in=";
  data_file_dir += std::filesystem::current_path();
  std::string data_file = "--vt_lb_data_file_in=";
  data_file += prefix + ".%p.json";

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(data_in);
  custom_args.emplace_back(const_cast<char*>(data_file_dir.c_str()));
  custom_args.emplace_back(const_cast<char*>(data_file.c_str()));
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  vt::initialize(custom_argc, custom_argv, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_lb_name, "NoLB");
  EXPECT_EQ(theConfig()->vt_lb_data_in, true);
  EXPECT_EQ(theConfig()->vt_lb_file_name, "");
  EXPECT_TRUE(theLBDataReader() == nullptr);
}

TEST_F(TestInitialization, test_initialize_with_lb_data_and_config_offline_lb) {
  MPI_Comm comm = MPI_COMM_WORLD;

  // Prepare data files
  auto prefix = getUniqueFilenameWithRanks();
  prepareLBDataFiles(prefix);

  // Prepare configuration file
  std::string file_name = getUniqueFilenameWithRanks(".txt");
  std::ofstream out(file_name);
  out << "0 OfflineLB\n";
  out.close();

  static char prog_name[]{"vt_program"};
  static char data_in[]{"--vt_lb_data_in"};
  std::string data_file_dir = "--vt_lb_data_dir_in=";
  data_file_dir += std::filesystem::current_path();
  std::string data_file = "--vt_lb_data_file_in=";
  data_file += prefix + ".%p.json";
  std::string config_file = "--vt_lb_file_name=" + file_name;

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(data_in);
  custom_args.emplace_back(const_cast<char*>(data_file_dir.c_str()));
  custom_args.emplace_back(const_cast<char*>(data_file.c_str()));
  custom_args.emplace_back(const_cast<char*>(config_file.c_str()));
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  vt::initialize(custom_argc, custom_argv, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_lb_name, "NoLB");
  EXPECT_EQ(theConfig()->vt_lb_data_in, true);
  EXPECT_EQ(theConfig()->vt_lb_file_name, file_name);
  EXPECT_TRUE(theLBDataReader() != nullptr);
}

TEST_F(TestInitialization, test_initialize_with_lb_data_and_no_lb) {
  MPI_Comm comm = MPI_COMM_WORLD;

  // Prepare data files
  auto prefix = getUniqueFilenameWithRanks();
  prepareLBDataFiles(prefix);

  // Prepare configuration file
  std::string file_name = getUniqueFilenameWithRanks(".txt");
  std::ofstream out(file_name);
  out << "0 NoLB\n";
  out.close();

  static char prog_name[]{"vt_program"};
  static char data_in[]{"--vt_lb_data_in"};
  std::string data_file_dir = "--vt_lb_data_dir_in=";
  data_file_dir += std::filesystem::current_path();
  std::string data_file = "--vt_lb_data_file_in=";
  data_file += prefix + ".%p.json";
  std::string config_file = "--vt_lb_file_name=" + file_name;

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(data_in);
  custom_args.emplace_back(const_cast<char*>(data_file_dir.c_str()));
  custom_args.emplace_back(const_cast<char*>(data_file.c_str()));
  custom_args.emplace_back(const_cast<char*>(config_file.c_str()));
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  vt::initialize(custom_argc, custom_argv, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_lb_name, "NoLB");
  EXPECT_EQ(theConfig()->vt_lb_data_in, true);
  EXPECT_EQ(theConfig()->vt_lb_file_name, file_name);
  EXPECT_TRUE(theLBDataReader() == nullptr);
}

TEST_F(TestInitialization, test_initialize_with_lb_data_and_offline_lb) {
  MPI_Comm comm = MPI_COMM_WORLD;

  // Prepare data files
  auto prefix = getUniqueFilenameWithRanks();
  prepareLBDataFiles(prefix);

  static char prog_name[]{"vt_program"};
  static char data_in[]{"--vt_lb_data_in"};
  static char offline_lb[]{"--vt_lb_name=OfflineLB"};
  std::string data_file_dir = "--vt_lb_data_dir_in=";
  data_file_dir += std::filesystem::current_path();
  std::string data_file = "--vt_lb_data_file_in=";
  data_file += prefix + ".%p.json";

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(data_in);
  custom_args.emplace_back(offline_lb);
  custom_args.emplace_back(const_cast<char*>(data_file_dir.c_str()));
  custom_args.emplace_back(const_cast<char*>(data_file.c_str()));
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  vt::initialize(custom_argc, custom_argv, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_lb_name, "OfflineLB");
  EXPECT_EQ(theConfig()->vt_lb_data_in, true);
  EXPECT_EQ(theConfig()->vt_lb_file_name, "");
  EXPECT_TRUE(theLBDataReader() != nullptr);
}
#endif

TEST_F(TestInitialization, test_initialize_with_lb_data_and_config_no_lb) {
  MPI_Comm comm = MPI_COMM_WORLD;

  // Prepare data files
  auto prefix = getUniqueFilenameWithRanks();
  prepareLBDataFiles(prefix);

  // Prepare configuration file
  std::string file_name = getUniqueFilenameWithRanks(".txt");
  std::ofstream out(file_name);
  out << "0 NoLB\n";
  out.close();

  static char prog_name[]{"vt_program"};
  static char data_in[]{"--vt_lb_data_in"};
  std::string data_file_dir = "--vt_lb_data_dir_in=";
  data_file_dir += std::filesystem::current_path();
  std::string data_file = "--vt_lb_data_file_in=";
  data_file += prefix + ".%p.json";
  std::string config_file = "--vt_lb_file_name=" + file_name;

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(data_in);
  custom_args.emplace_back(const_cast<char*>(data_file_dir.c_str()));
  custom_args.emplace_back(const_cast<char*>(data_file.c_str()));
  custom_args.emplace_back(const_cast<char*>(config_file.c_str()));
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  vt::initialize(custom_argc, custom_argv, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_lb_name, "NoLB");
  EXPECT_EQ(theConfig()->vt_lb_data_in, true);
  EXPECT_EQ(theConfig()->vt_lb_file_name, file_name);
  EXPECT_TRUE(theLBDataReader() == nullptr);
}

TEST_F(TestInitialization, test_initialize_with_yaml_toml_and_args) {
  MPI_Comm comm = MPI_COMM_WORLD;

  // Set command line arguments
  static char prog_name[]{"vt_program"};
  static char vt_debug_level[]{"--vt_debug_level=verbose"};

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(vt_debug_level);

  // Set TOML config file
  std::string toml_config_file(getUniqueFilenameWithRanks(".toml"));
  std::string toml_config_flag("--vt_input_config=");
  std::string vt_input_config_toml = toml_config_flag + toml_config_file;

  custom_args.emplace_back(strdup(vt_input_config_toml.c_str()));

  int this_rank;
  MPI_Comm_rank(comm, &this_rank);

  if (this_rank == 0) {
    std::ofstream toml_cfg_file_{toml_config_file.c_str(), std::ofstream::out | std::ofstream::trunc};
    toml_cfg_file_ << "vt_debug_level = terse\n"
                   << "vt_color = False";
    toml_cfg_file_.close();
  }
  MPI_Barrier(comm);

  // Set YAML config file
  std::string yaml_config_file(getUniqueFilenameWithRanks(".yaml"));
  std::string yaml_config_flag("--vt_input_config_yaml=");
  std::string vt_input_config_yaml = yaml_config_flag + yaml_config_file;

  custom_args.emplace_back(strdup(vt_input_config_yaml.c_str()));
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char** custom_argv = custom_args.data();

  if (this_rank == 0) {
    std::ofstream yaml_cfg_file_{yaml_config_file.c_str(), std::ofstream::out | std::ofstream::trunc};
    yaml_cfg_file_ << R"(
    Output Control:
      Color: True
      Quiet: True
    Debug Print Configuration:
      Level: normal
    )";
    yaml_cfg_file_.close();
  }

  MPI_Barrier(comm);

  vt::initialize(custom_argc, custom_argv, &comm);

  // Test that everything was read in correctly
  EXPECT_EQ(theConfig()->prog_name, "vt_program");

  EXPECT_EQ(theConfig()->vt_quiet, true);            // yaml
  EXPECT_EQ(theConfig()->vt_color, false);           // toml overwrites yaml
  EXPECT_EQ(theConfig()->vt_debug_level, "verbose"); // args overwrite everything
}

}}} // end namespace vt::tests::unit
