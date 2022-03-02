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

#include <vt/collective/startup.h>

#include <fstream>

namespace vt { namespace tests { namespace unit {

struct TestInitialization : TestParallelHarness { };

TEST_F(TestInitialization, test_initialize_with_args) {
  MPI_Comm comm = MPISingletonMultiTest::Get()->getComm();

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

  vt::initialize(custom_argc, custom_argv, no_workers, true, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_no_terminate, true);

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

TEST_F(TestInitialization, test_initialize_with_appconfig) {
  MPI_Comm comm = MPISingletonMultiTest::Get()->getComm();

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
  appConfig.vt_lb_stats = true;

  vt::initialize(custom_argc, custom_argv, no_workers, true, &comm, &appConfig);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_epoch_graph_on_hang, false);
  EXPECT_EQ(theConfig()->vt_lb_name, "RotateLB");
  EXPECT_EQ(theConfig()->vt_lb_stats, true);

  // vt_no_detect_hang wasn't set, should be default
  EXPECT_EQ(theConfig()->vt_no_detect_hang, false);

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

TEST_F(TestInitialization, test_initialize_with_args_and_appconfig) {
  MPI_Comm comm = MPISingletonMultiTest::Get()->getComm();

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
  appConfig.vt_lb_stats = true;
  appConfig.vt_no_detect_hang = false;

  vt::initialize(custom_argc, custom_argv, no_workers, true, &comm, &appConfig);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_color, false);
  EXPECT_EQ(theConfig()->vt_epoch_graph_on_hang, false);
  EXPECT_EQ(theConfig()->vt_lb_name, "RotateLB");
  EXPECT_EQ(theConfig()->vt_lb_stats, true);
  EXPECT_EQ(theConfig()->vt_no_terminate, true);
  // CLI args should overwrite hardcoded appConfig
  EXPECT_EQ(theConfig()->vt_no_detect_hang, true);

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

TEST_F(TestInitialization, test_initialize_with_file_and_args) {
  MPI_Comm comm = MPISingletonMultiTest::Get()->getComm();

  static char prog_name[]{"vt_program"};
  static char cli_argument[]{"--cli_argument=100"};
  static char vt_no_terminate[]{"--vt_no_terminate"};
  static char vt_lb_name[]{"--vt_lb_name=RotateLB"};
  static char vt_input_config[]{"--vt_input_config=test_cfg.toml"};

  std::vector<char *> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(cli_argument);
  custom_args.emplace_back(vt_no_terminate);
  custom_args.emplace_back(vt_input_config);
  custom_args.emplace_back(vt_lb_name);
  custom_args.emplace_back(nullptr);

  int custom_argc = custom_args.size() - 1;
  char **custom_argv = custom_args.data();

  EXPECT_EQ(custom_argc, 5);

  int this_rank;
  MPI_Comm_rank(comm, &this_rank);
  if (this_rank == 0) {
    std::ofstream cfg_file_{"test_cfg.toml", std::ofstream::out | std::ofstream::trunc};
    cfg_file_ << "vt_lb_name = RandomLB\n";
    cfg_file_.close();
  }
  MPI_Barrier(comm);

  vt::initialize(custom_argc, custom_argv, no_workers, true, &comm);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_no_terminate, true);
  EXPECT_EQ(theConfig()->vt_lb_name, "RotateLB");

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

TEST_F(TestInitialization, test_initialize_with_file_args_and_appconfig) {
  MPI_Comm comm = MPISingletonMultiTest::Get()->getComm();

  static char prog_name[]{"vt_program"};
  static char cli_argument[]{"--cli_argument=100"};
  static char vt_no_terminate[]{"--vt_no_terminate"};
  static char vt_lb_name[]{"--vt_lb_name=RotateLB"};
  static char vt_input_config[]{"--vt_input_config=test_cfg.toml"};

  std::vector<char*> custom_args;
  custom_args.emplace_back(prog_name);
  custom_args.emplace_back(cli_argument);
  custom_args.emplace_back(vt_no_terminate);
  custom_args.emplace_back(vt_input_config);
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
    std::ofstream cfg_file_{"test_cfg.toml", std::ofstream::out | std::ofstream::trunc};
    cfg_file_ << "vt_lb_name = RandomLB\n";
    cfg_file_.close();
  }
  MPI_Barrier(comm);

  vt::initialize(custom_argc, custom_argv, no_workers, true, &comm, &appConfig);

  EXPECT_EQ(theConfig()->prog_name, "vt_program");
  EXPECT_EQ(theConfig()->vt_no_terminate, true);
  EXPECT_EQ(theConfig()->vt_lb_name, "RotateLB");

  EXPECT_EQ(custom_argc, 2);
  EXPECT_STREQ(custom_argv[0], "vt_program");
  EXPECT_STREQ(custom_argv[1], "--cli_argument=100");
  EXPECT_EQ(custom_argv[2], nullptr);
}

}}} // end namespace vt::tests::unit
