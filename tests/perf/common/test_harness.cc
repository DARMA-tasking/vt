/*
//@HEADER
// *****************************************************************************
//
//                               test_harness.cc
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

#include "test_harness.h"

#include <vt/collective/collective_ops.h>
#include <vt/scheduler/scheduler.h>
#include <vt/utils/memory/memory_usage.h>

#include <numeric>

namespace vt { namespace tests { namespace perf { namespace common {

static void CopyTestData(
  PerfTestHarness::TestResults const& source,
  PerfTestHarness::CombinedResults& dest, NodeType node
) {
  for (auto const& test_result : source) {
    auto const& test_name = test_result.first;
    auto& dst = dest[test_name][node];
    auto const& src = test_result.second;

    std::copy(src.begin(), src.end(), std::back_inserter(dst));
  }
}

struct TestMsg : Message {
  vt_msg_serialize_required();
  TestMsg() = default;


  TestMsg(PerfTestHarness::TestResults const& results, NodeType from_node)
    : results_(results),
      from_node_(from_node) { }

  PerfTestHarness::TestResults results_ = {};
  NodeType from_node_ = {};

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    Message::serialize(s);
    s | results_;
    s | from_node_;
  }
};

struct OutputGenerator{

};
/*
* Helper function which generates CSV file with the tests resutls.
* Example output:
* name,min,max,mean,
*
*/
void OutputToFile() { }

PerfTestHarness::CombinedResults PerfTestHarness::combined_timings_ = {};

void PerfTestHarness::SetUp(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv, no_workers, true);
  my_node_ = theContext()->getNode();

  // DumpResults (which uses colorized output) will be called
  // after vt finalizes, so we use preconfig here
  vt::debug::preConfigRef()->colorize_output = true;
}

void PerfTestHarness::TearDown() {
  SpinScheduler();
  CollectiveOps::finalize();

  if(my_node_ != 0){
    timings_.clear();
  }
}

std::string PerfTestHarness::GetName() const {
  return name_;
}

void PerfTestHarness::DumpResults() const {
  // Only dump results on root node
  if (my_node_ == 0) {
    // Copy the root node's data to 'combined_timings_'
    CopyTestData(timings_, combined_timings_, my_node_);

    for (auto const& test_run : combined_timings_) {
      auto const name = test_run.first;

      for (auto const& per_node_result : test_run.second) {
        auto const node = per_node_result.first;
        auto const& timings = per_node_result.second;

        auto const num_timings = timings.size();
        auto const mean =
          std::accumulate(timings.begin(), timings.end(), 0.0) / num_timings;

        fmt::print(
          "{} Timings for {} (mean value: {})\n", debug::proc(node),
          debug::reg(name), debug::emph(fmt::format("{:.3f}ms", mean)));

        for (uint32_t run_num = 0; run_num < num_timings; ++run_num) {
          fmt::print(
            "{} Run {} -> {}\n", debug::proc(node), run_num,
            debug::emph(fmt::format("{:.3f}ms", timings[run_num])));
        }
      }
    }
  }
}

void PerfTestHarness::AddResult(TestResult const& test_result, bool iteration_finished) {
  auto found = timings_.find(test_result.first);

  if (found != timings_.end()) {
    found->second.push_back(test_result.second);
  } else {
    timings_[test_result.first] = {test_result.second};
  }

  // Root node will be responsible for generating the final output
  // so every other node sends its results to it (at the end of iteration)
  if (iteration_finished) {
    runInEpochCollective([this] {
      constexpr auto root_node = 0;
      if (my_node_ != root_node) {
        auto msg = makeMessage<TestMsg>(timings_, my_node_);
        theMsg()->sendMsg<TestMsg, &PerfTestHarness::RecvTestResult>(
          root_node, msg);
      }
    });
  }
}

void PerfTestHarness::RecvTestResult(TestMsg* msg) {
  fmt::print(
    "Received test result from node = {} with {} tests results!\n",
    msg->from_node_, msg->results_.size());

  CopyTestData(msg->results_, combined_timings_, msg->from_node_);
}

void PerfTestHarness::SpinScheduler() {
  vt::theSched()->runSchedulerWhile([] { return !rt->isTerminated(); });
}

void PerfTestHarness::StartTimer() {
  watch_.Start();
}

void PerfTestHarness::StopTimer() {
  auto const time_elapsed = watch_.Stop();
  fmt::print("Node:{} {} took {}ms\n", my_node_, name_, time_elapsed);
}

void PerfTestHarness::GetMemoryUsage() {
  auto const mem_usage = vt::theMemUsage()->getUsageAll();
  fmt::print("Node:{} {} memory usage {}\n", my_node_, "name", mem_usage);
}

}}}} // namespace vt::tests::perf::common
