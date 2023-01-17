/*
//@HEADER
// *****************************************************************************
//
//                           test_parallel_harness.h
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

#if ! defined INCLUDED_UNIT_TEST_PARALLEL_HARNESS_H
#define INCLUDED_UNIT_TEST_PARALLEL_HARNESS_H

#include <vector>
#include <mpi.h>

#include "test_config.h"
#include "test_harness.h"

#include "vt/collective/collective_ops.h"
#include "vt/scheduler/scheduler.h"

namespace vt { namespace tests { namespace unit {

extern int test_argc;
extern char** test_argv;

template <typename TestBase>
struct TestParallelHarnessAny : TestHarnessAny<TestBase> {
  virtual void SetUp() {
    using namespace vt;

    TestHarnessAny<TestBase>::SetUp();

#if vt_feature_cmake_test_trace_on
    static char traceon[]{"--vt_trace=1"};
    addArgs(traceon);
#endif

    static char throw_on_abort[]{"--vt_throw_on_abort=1"};
    addArgs(throw_on_abort);

    // initialize MPI if it hasn't already happened
    int init = 0;
    MPI_Initialized(&init);
    if (!init) {
      MPI_Init(&test_argc, &test_argv);
    }
    MPI_Comm comm = MPI_COMM_WORLD;
    auto const new_args = injectAdditionalArgs(test_argc, test_argv);
    auto custom_argc = new_args.first;
    auto custom_argv = new_args.second;
    vtAssert(
      custom_argv[custom_argc] == nullptr,
      "The value of argv[argc] should always be 0"
    );
    // communicator is duplicated.
    CollectiveOps::initialize(custom_argc, custom_argv, true, &comm);

#if DEBUG_TEST_HARNESS_PRINT
    auto const& my_node = theContext()->getNode();
    auto const& num_nodes = theContext()->getNumNodes();
    fmt::print("my_node={}, num_nodes={}\n", my_node, num_nodes);
#endif
  }

  virtual void TearDown() {
    using namespace vt;

    try {
      vt::theSched()->runSchedulerWhile([] { return !rt->isTerminated(); });
    } catch (std::exception& e) {
      ADD_FAILURE() << fmt::format("Caught an exception: {}\n", e.what());
    }

#if DEBUG_TEST_HARNESS_PRINT
    auto const& my_node = theContext()->getNode();
    fmt::print("my_node={}, tearing down runtime\n", my_node);
#endif

    CollectiveOps::finalize();

    TestHarnessAny<TestBase>::TearDown();
  }

protected:
  template <typename Arg>
  void addArgs(Arg& arg) {
    this->additional_args_.emplace_back(&arg[0]);
  }

  template <typename Arg, typename... Args>
  void addArgs(Arg& arg, Args&... args) {
    this->additional_args_.emplace_back(&arg[0]);
    addArgs(args...);
  }

private:
  std::pair<int, char**>
  injectAdditionalArgs(int old_argc, char** old_argv) {
    additional_args_.insert(
      additional_args_.begin(), old_argv, old_argv + old_argc
    );

    addAdditionalArgs();

    additional_args_.emplace_back(nullptr);
    int custom_argc = additional_args_.size() - 1;
    char** custom_argv = additional_args_.data();

    return std::make_pair(custom_argc, custom_argv);
  }

  /**
   * \internal \brief Add additional arguments used during initialization of vt
   * components
   *
   * To add additional arguments override this function in your class and add
   * needed arguments to `additional_args_` vector.
   *
   * Example:
   * struct TestParallelHarnessWithLBDataDumping : TestParallelHarnessParam<int> {
   *   virtual void addAdditionalArgs() override {
   *     static char vt_lb_data[]{"--vt_lb_data"};
   *     static char vt_lb_data_dir[]{"--vt_lb_data_dir=test_data_dir"};
   *     static char vt_lb_data_file[]{"--vt_lb_data_file=test_data_outfile"};
   *
   *     addArgs(vt_lb_data, vt_lb_data_dir, vt_lb_data_file);
   *   }
   * };
   *
   * Make sure all filenames used will be unique across all tests,
   * parameterizations, and MPI rank counts.
   */
  virtual void addAdditionalArgs() {}

  std::vector<char*> additional_args_;
};

using TestParallelHarness = TestParallelHarnessAny<testing::Test>;

template <typename ParamT>
using TestParallelHarnessParam = TestParallelHarnessAny<
  testing::TestWithParam<ParamT>
>;

using TestParameterHarnessNode = TestParallelHarnessParam<vt::NodeType>;

}}} // end namespace vt::tests::unit

#endif /* INCLUDED_UNIT_TEST_PARALLEL_HARNESS_H */
