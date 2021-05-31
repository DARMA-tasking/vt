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
#include <unordered_map>
#include <vector>

namespace vt { namespace tests { namespace perf { namespace common {

struct TestMsg;

struct PerfTestHarness {
  using TestResults = std::unordered_map<std::string, std::vector<double>>;
  using CombinedResults = std::unordered_map<
    std::string, std::unordered_map<NodeType, std::vector<double>>>;
  using TestResult = std::pair<std::string, double>;

  virtual ~PerfTestHarness() = default;

  virtual void SetUp(int argc, char** argv);
  virtual void TearDown();

  /**
   * \brief Get the name if this test suite
   *
   * \return name
   */
  std::string GetName() const;

  /**
   * \brief Dump the test results to stdout and CSV file ({name_}.csv)
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

  void GetMemoryUsage();

  protected:
  static NodeType my_node_;
  static std::unordered_map<std::string, StopWatch> timers_;
  static std::string name_;

  // Local (per node) timings
  static TestResults timings_;

  // Combined timings from all nodes, that are stored on the root node
  static CombinedResults combined_timings_;
};

}}}} // namespace vt::tests::perf::common

#endif // __VIRTUAL_TRANSPORT_TEST_PERFORMANCE_PARALLEL_HARNESS__
