/*
//@HEADER
// *****************************************************************************
//
//                               test_harness.h
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

#if !defined __VIRTUAL_TRANSPORT_TEST_PERFORMANCE_PARALLEL_HARNESS__
#define __VIRTUAL_TRANSPORT_TEST_PERFORMANCE_PARALLEL_HARNESS__

#include "timers.h"
#include "test_harness_macros.h"

#include <vt/configs/types/types_type.h>
#include <vt/utils/memory/memory_usage.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace vt { namespace tests { namespace perf { namespace common {

struct TestMsg;

struct PerfTestHarness {
  using TestResult = std::pair<std::string, TimeDuration>;
  using TestResults = std::unordered_map<std::string, std::vector<TimeDuration>>;
  using PerNodeResults = std::unordered_map<NodeType, std::vector<TimeDuration>>;
  using CombinedResults = std::unordered_map<std::string, PerNodeResults>;

  // Memory use at the end of test iteration (i.e. phase)
  using MemoryUsage = std::vector<std::size_t>;
  using CombinedMemoryUse = std::unordered_map<NodeType, MemoryUsage>;

  virtual ~PerfTestHarness() = default;

  virtual void SetUp(int argc, char** argv);
  virtual void TearDown();

  /**
   * \brief Get the name of this test suite
   *
   * \return name
   */
  std::string GetName() const;

  /**
   * \brief Dump the test results to stdout and CSV files.
   * {name_}_mem.csv file which contains memory usage for each iteration
   * {name_}_time.csv file which contains all time-related data
   *
   * This is called at the end of running test suite.
   */
  void DumpResults() const;

  /**
   * \brief Add a single test result (name-time pair)
   *
   * \param[in] test_result name-time pair of test result
   */
  static void AddResult(TestResult const& test_result);

  /**
   * \brief Send the tests' results to root node.
   * This is called after each test iteration.
   */
  static void SyncResults();

  /**
   * \brief Handler for receiving test results from other nodes
   *
   * \param[in] msg message that contains the results
   */
  static void RecvTestResult(TestMsg* msg);

  /**
   * \brief Spin the scheduler until all work is done.
   */
  static void SpinScheduler();

  /**
   * \brief Helper function used for phase-based tests.
   * This will register for Begin/End Phase notifications from \c PhaseManager
   * and uses \c StopWatch to measure time
   */
  static void BenchmarkPhase(std::string const& prefix = "phase");

  /**
   * \brief Helper function used for tracking memory usage.
   * Should be called each iteration. Uses \c StatM to track memory usage
   */
  static void GetMemoryUsage();

  private:
  std::string OutputMemoryUse() const;
  std::string OutputTimeResults() const;

  protected:
  bool gen_file_ = false;
  bool verbose_ = false;
  static NodeType my_node_;

  // Memory usage (in bytes) per iteration
  static MemoryUsage memory_use_;
  static vt::util::memory::StatM mem_tracker_;

  // Test suite name
  static std::string name_;

  // Local (per node) timings.
  static TestResults timings_;
  static std::unordered_map<std::string, StopWatch> timers_;

  // Combined timings from all nodes, that are stored on the root node
  static CombinedResults combined_timings_;
  static CombinedMemoryUse combined_mem_use_;
};

}}}} // namespace vt::tests::perf::common

#endif // __VIRTUAL_TRANSPORT_TEST_PERFORMANCE_PARALLEL_HARNESS__
