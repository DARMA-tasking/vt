/*
//@HEADER
// *****************************************************************************
//
//                        test_cli_arguments.extended.cc
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

#include <vt/configs/arguments/app_config.h>
#include <vt/configs/arguments/args.h>

namespace vt { namespace tests { namespace unit {

struct TestCliArguments : TestParallelHarness { };

#if not vt_check_enabled(production_build)
TEST_F(TestCliArguments, test_vt_assert) {
  EXPECT_EQ(theConfig()->vt_no_assert_fail, false);

  ASSERT_THROW(
    vtAssert(false, "Should throw."),
    std::runtime_error
  );
}
#endif

TEST_F(TestCliArguments, test_assert_no_fail) {
  EXPECT_EQ(theConfig()->vt_no_assert_fail, false);
  theConfig()->vt_no_assert_fail = true;

  vtAssert(false, "Should not abort.");
  SUCCEED();
}

TEST_F(TestCliArguments, test_seperating_args_invalid_params) {
  std::vector<char*> custom_args;
  int argc = 0;
  char** argv = custom_args.data();

  EXPECT_EQ(argc, 0);
  auto pair = vt::arguments::separateParameters(argc, argv);
  EXPECT_EQ(0, pair.first.size());
  EXPECT_EQ(0, pair.second.size());
}

TEST_F(TestCliArguments, test_seperating_args_no_params) {
  char program_name[] = "some_program";
  std::vector<char*> custom_args;
  custom_args.emplace_back(program_name);
  custom_args.emplace_back(nullptr);

  int argc = custom_args.size() - 1;
  char** argv = custom_args.data();

  EXPECT_EQ(argc, 1);
  auto pair = vt::arguments::separateParameters(argc, argv);
  EXPECT_EQ(0, pair.first.size());
  EXPECT_EQ(0, pair.second.size());
}

TEST_F(TestCliArguments, test_seperating_args_non_vt_params) {
  char program_name[] = "some_program";
  char non_vt_1[] = "--non_vt_arg=123";
  char non_vt_2[] = "--foo=bar";
  std::vector<char*> custom_args;
  custom_args.emplace_back(program_name);
  custom_args.emplace_back(non_vt_1);
  custom_args.emplace_back(non_vt_2);
  custom_args.emplace_back(nullptr);

  int argc = custom_args.size() - 1;
  char** argv = custom_args.data();

  EXPECT_EQ(argc, 3);
  auto pair = vt::arguments::separateParameters(argc, argv);
  EXPECT_EQ(0, pair.first.size());
  EXPECT_EQ(2, pair.second.size());
  EXPECT_EQ(pair.second[0], "--non_vt_arg=123");
  EXPECT_EQ(pair.second[1], "--foo=bar");
}

TEST_F(TestCliArguments, test_seperating_args) {
  char program_name[] = "vt";
  char non_vt_1[] = "--non_vt_arg=123";
  char vt_1[] = "--vt_debug_level";
  char non_vt_2[] = "--foo=bar";
  char vt_2[] = "!--vt_some_param=1000";

  std::vector<char*> custom_args;
  custom_args.emplace_back(program_name);
  custom_args.emplace_back(non_vt_1);
  custom_args.emplace_back(vt_1);
  custom_args.emplace_back(non_vt_2);
  custom_args.emplace_back(vt_2);
  custom_args.emplace_back(nullptr);

  int argc = custom_args.size() - 1;
  char** argv = custom_args.data();

  EXPECT_EQ(argc, 5);

  auto pair = vt::arguments::separateParameters(argc, argv);
  EXPECT_EQ(2, pair.first.size());
  EXPECT_EQ(pair.first[0], "--vt_debug_level");
  EXPECT_EQ(pair.first[1], "!--vt_some_param=1000");
  EXPECT_EQ(2, pair.second.size());
  EXPECT_EQ(pair.second[0], "--non_vt_arg=123");
  EXPECT_EQ(pair.second[1], "--foo=bar");
}

TEST_F(TestCliArguments, test_seperating_args_2) {
  char program_name[] = "vt";
  char vt_1[] = "--vt_debug_level";
  char vt_2[] = "!--vt_some_param=1000";
  char other_program[] = "some_program";
  char non_vt_1[] = "--non_vt_arg=123";
  char non_vt_2[] = "--foo=bar";

  std::vector<char*> custom_args;
  custom_args.emplace_back(program_name);
  custom_args.emplace_back(vt_1);
  custom_args.emplace_back(vt_2);
  custom_args.emplace_back(other_program);
  custom_args.emplace_back(non_vt_1);
  custom_args.emplace_back(non_vt_2);
  custom_args.emplace_back(nullptr);

  int argc = custom_args.size() - 1;
  char** argv = custom_args.data();

  EXPECT_EQ(argc, 6);

  auto pair = vt::arguments::separateParameters(argc, argv);
  EXPECT_EQ(2, pair.first.size());
  EXPECT_EQ(pair.first[0], "--vt_debug_level");
  EXPECT_EQ(pair.first[1], "!--vt_some_param=1000");
  EXPECT_EQ(3, pair.second.size());
  EXPECT_EQ(pair.second[0], "some_program");
  EXPECT_EQ(pair.second[1], "--non_vt_arg=123");
  EXPECT_EQ(pair.second[2], "--foo=bar");
}

}}} // end namespace vt::tests::unit
