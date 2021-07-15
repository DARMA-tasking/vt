/*
//@HEADER
// *****************************************************************************
//
//                                test_harness.h
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

#if !defined INCLUDED_PERF_COMMON_TEST_HARNESS_H
#define INCLUDED_PERF_COMMON_TEST_HARNESS_H

#include "timers.h"
#include "test_harness_macros.h"

#include <vt/configs/types/types_type.h>
#include <vt/utils/memory/memory_usage.h>

#include <unordered_map>
#include <string>
#include <vector>

namespace vt { namespace tests { namespace perf { namespace common {

template <typename T>
struct TestResultHolder {
  T mean_ = {};
  T std_dev_ = {};
  T min_ = std::numeric_limits<T>::max();
  T max_ = {};
};

struct PerfTestHarness {
  using TestName = std::string;
  using TestResult = std::pair<TestName, TimeType>;
  using FinalTestResult = std::pair<TestName, TestResultHolder<TimeType>>;
  using TestResults = std::vector<std::vector<TestResult>>;
  using PerNodeResults =
    std::unordered_map<NodeType, TestResultHolder<TimeType>>;
  using CombinedResults = std::vector<std::pair<TestName, PerNodeResults>>;

  // Memory use at the end of test iteration (i.e. phase)
  using MemoryUsage = std::vector<std::vector<std::size_t>>;
  using CombinedMemoryUse =
    std::unordered_map<NodeType, std::vector<TestResultHolder<std::size_t>>>;

  virtual ~PerfTestHarness() = default;

  virtual void SetUp();
  virtual void TearDown();

  /**
   * \brief Initialize internal variables and parse args
   * Perf specific args:
   * --vt_perf_gen_file - generate .CSV files with test results
   * --vt_perf_verbose  - output per iteration times/memory use
   * --vt_perf_num_runs - set how many times the tests should run
   *                      (e.g. --vt_perf_num_run=100)
   */
  void Initialize(int argc, char** argv);

  /**
   * \brief Get the name of this test suite
   *
   * \return name
   */
  std::string GetName() const;

  /**
   * \brief Get the number of runs that this test will run
   *
   * \return Number of runs
   */
  uint32_t GetNumRuns() const;

  /**
   * \brief Dump the test results to stdout and CSV files.
   * {name_}_mem.csv file which contains memory usage for each iteration
   * {name_}_time.csv file which contains all time-related data
   *
   * This is called at the end of running test suite.
   */
  void DumpResults();

  /**
   * \brief Add a single test result (name-time pair)
   *
   * \param[in] test_result name-time pair of test result
   */
  void AddResult(TestResult const& test_result);

  /**
   * \brief Add and start a timer with name \c name
   *
   * \param[in] name name of the timer
   */
  void StartTimer(std::string const& name);

  /**
   * \brief Stop the timer \c name and add test result.
   * This function calls \c AddResult
   *
   * \param[in] name name of the timer
   */
  void StopTimer(std::string const& name);

  /**
   * \brief Send the tests' results to root node.
   * This is called after each test run.
   */
  void SyncResults();

  /**
   * \brief Copies the test results into combined structures.
   * Called at the end of all test runs (on root node)
   *
   * \param[in] timers time results from all test runs
   * \param[in] memory_usage memory usage from all test runs
   * \param[in] from_node which node sent these results
   */
  void CopyTestData(
    PerfTestHarness::TestResults const& timers,
    PerfTestHarness::MemoryUsage const& memory_usage, NodeType const from_node
  );

  /**
   * \brief Spin the scheduler until all work is done.
   */
  static void SpinScheduler();

  /**
   * \brief Helper function used for tracking memory usage.
   * Should be called each iteration. Uses \c StatM to track memory usage
   */
  void GetMemoryUsage();

private:
  std::string OutputMemoryUse() const;
  std::string OutputTimeResults();

protected:
  bool gen_file_ = false;
  bool verbose_ = false;
  uint32_t num_runs_ = 50;
  uint32_t current_run_ = 0;
  std::vector<char*> custom_args_ = {};

  NodeType my_node_ = uninitialized_destination;
  NodeType num_nodes_ = uninitialized_destination;
  std::string name_ = {};

  // Memory usage (in bytes) per iteration
  MemoryUsage memory_use_ = {};
  vt::util::memory::StatM mem_tracker_ = {};

  // Local (per node) timings.
  TestResults timings_ = {};
  std::unordered_map<std::string, StopWatch> timers_ = {};

  // Combined timings from all nodes, that are stored on the root node
  CombinedResults combined_timings_;
  CombinedMemoryUse combined_mem_use_;
};

}}}} // namespace vt::tests::perf::common

#endif /*INCLUDED_PERF_COMMON_TEST_HARNESS_H*/
