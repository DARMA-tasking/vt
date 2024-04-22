/*
//@HEADER
// *****************************************************************************
//
//                            test_initialization.cc
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
#include "test_helpers.h"

#include <vt/collective/startup.h>
#include <vt/vrt/collection/balance/lb_data_restart_reader.h>
#include <vt/utils/json/json_appender.h>
#include <vt/elm/elm_id_bits.h>

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

TEST_F(TestInitialization, test_initialize_with_file_and_args) {
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

TEST_F(TestInitialization, test_initialize_with_file_args_and_appconfig) {
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

TEST_F(TestInitialization, test_preconfigure_and_initialization) {
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

  auto vtConfig = vt::preconfigure(custom_argc, custom_argv);

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);

  vt::initializePreconfigured(&comm, &appConfig);

  EXPECT_EQ(theConfig()->prog_name, "vt_unknown");
  EXPECT_EQ(theConfig()->vt_no_terminate, false);
  EXPECT_EQ(theConfig()->vt_lb_name, "GreedyLB");

  vt::finalize();
  vt::initializePreconfigured(&comm, &appConfig, vtConfig.get());

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_no_terminate, true);
  EXPECT_EQ(theConfig()->vt_lb_name, "RotateLB");

  vt::finalize();
  vt::initializePreconfigured(&comm, &appConfig, vtConfig.get());
}

void preapreLBDataFiles(const std::string file_name_without_ext) {
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
  for (PhaseType i = 0; i < num_phases; i++) {
    for (auto&& elm : ids[i]) {
      dh.node_data_[i][elm] = LoadSummary{3};
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

  // save to files
  for (int i = 0; i < 9; i++) {
    std::string file_name = file_name_without_ext + "." + std::to_string(i) + ".json";
    std::filesystem::path file_path = std::filesystem::current_path() / file_name;
    std::ofstream out(file_path);
    out << stream.str();
    out.close();
  }
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
  EXPECT_TRUE(theLBDataReader() == nullptr);
}

#if vt_feature_cmake_lblite
TEST_F(TestInitialization, test_initialize_with_lb_data_in) {
  MPI_Comm comm = MPI_COMM_WORLD;

  // Preapre data files
  auto prefix = getUniqueFilenameWithRanks() + std::to_string(theContext()->getNode());
  preapreLBDataFiles(prefix);

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
  EXPECT_EQ(theConfig()->vt_lb_data_in, true);
  EXPECT_TRUE(theLBDataReader() != nullptr);
}

TEST_F(TestInitialization, test_initialize_with_lb_data_and_config_offline_lb) {
  MPI_Comm comm = MPI_COMM_WORLD;

  // Preapre data files
  auto prefix = getUniqueFilenameWithRanks() + std::to_string(theContext()->getNode());
  preapreLBDataFiles(prefix);

  // Preapre configuration file
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
  EXPECT_EQ(theConfig()->vt_lb_data_in, true);
  EXPECT_EQ(theConfig()->vt_lb_file_name, file_name);
  EXPECT_TRUE(theLBDataReader() != nullptr);
}
#endif

TEST_F(TestInitialization, test_initialize_with_lb_data_and_config_no_lb) {
  MPI_Comm comm = MPI_COMM_WORLD;

  // Preapre data files
  auto prefix = getUniqueFilenameWithRanks() + std::to_string(theContext()->getNode());
  preapreLBDataFiles(prefix);

  // Preapre configuration file
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
  EXPECT_EQ(theConfig()->vt_lb_data_in, true);
  EXPECT_EQ(theConfig()->vt_lb_file_name, file_name);
  EXPECT_TRUE(theLBDataReader() == nullptr);
}

}}} // end namespace vt::tests::unit
