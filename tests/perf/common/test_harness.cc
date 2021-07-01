/*
//@HEADER
// *****************************************************************************
//
//                               test_harness.cc
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

#include "test_harness.h"

#include <vt/collective/collective_ops.h>
#include <vt/scheduler/scheduler.h>
#include <vt/phase/phase_manager.h>
#include <vt/utils/memory/memory_usage.h>
#include <vt/objgroup/manager.h>

#include <numeric>
#include <fstream>

namespace vt { namespace tests { namespace perf { namespace common {

/////////////////////////////////////////////////
///////////////      HELPERS      ///////////////
/////////////////////////////////////////////////

/**
 * \brief Prints memory usage in human readable format
 */
static std::string GetFormattedMemUsage(std::size_t memory) {
  auto const& best_mem = util::memory::getBestMemoryUnit(memory);

  return fmt::format(
    "{}",
    debug::emph(
      fmt::format("{:.6g}{}", std::get<1>(best_mem), std::get<0>(best_mem))
    )
  );
}

/**
 * \brief Helper function that converts the test results
 * from all runs into a final vector, which contains
 * mean/min/max values for each test result.
 *
 * \param[in] input_vec vector that contains test data (memory usage or timers)
 * from all test runs. By default all tests are run 50 times
 * (see --vt_perf_num_runs flag) so this vector will have 50 elems
 * and each element is a vector of test results for that run.
 *
 * \param[in] populate function that will be called (for each test run)
 * in order to calculate mean/min/max.
 * Takes three params:
 *  1. Final test result
 *  2. Test result for given test run
 *  3. Number of test runs
 *
 * \param[in] std_dev function that will be called (for each test run)
 * in order to calculate standard deviation.
 * Takes four params:
 *  1. Final test result
 *  2. Test result for given test run
 *  3. Number of test runs
 *  4. Whether it's the last test run iteration
 *
 * \return Vector with final test results (mean/std_dev/min/max)
 */
template <typename InputT, typename OutputT>
std::vector<OutputT> ProcessInput(
  std::vector<std::vector<InputT>> const& input_vec,
  std::function<void(OutputT&, InputT const&, std::size_t const)> populate,
  std::function<void(OutputT&, InputT const&, std::size_t const, bool const)>
    std_dev
  )
{
  auto const vec_size = input_vec.front().size();
  auto const num_runs = input_vec.size();
  std::vector<OutputT> values_vec(vec_size, OutputT{});

  // Calculate mean/min/max
  for (auto const& per_run_result : input_vec) {
    for (std::decay_t<decltype(vec_size)> i = 0; i < vec_size; ++i) {
      populate(values_vec[i], per_run_result[i], num_runs);
    }
  }

  // Calculate variance and standard deviation
  for (std::decay_t<decltype(num_runs)> i = 0; i < num_runs; ++i) {
    for (std::decay_t<decltype(vec_size)> j = 0; j < vec_size; ++j) {
      auto const is_last_elem = i == (num_runs - 1);
      std_dev(values_vec[j], input_vec[i][j], num_runs, is_last_elem);
    }
  }

  return values_vec;
}

struct TestMsg : Message {
  vt_msg_serialize_required();
  TestMsg() = default;

  TestMsg(
    PerfTestHarness::TestResults const& results,
    PerfTestHarness::MemoryUsage const& memory, NodeType const from_node)
    : results_(results),
      memory_load_(memory),
      from_node_(from_node) { }

  PerfTestHarness::TestResults results_ = {};
  PerfTestHarness::MemoryUsage memory_load_ = {};
  NodeType from_node_ = {};

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    Message::serialize(s);
    s | results_;
    s | memory_load_;
    s | from_node_;
  }
};

struct TestNodeObj {
  explicit TestNodeObj(PerfTestHarness* test_harness)
    : test_harness_(test_harness) { }

  void RecvTestResult(TestMsg* msg) {
    test_harness_->CopyTestData(
      msg->results_, msg->memory_load_, msg->from_node_
    );
  }

  PerfTestHarness* test_harness_ = nullptr;
};

void OutputToFile(std::string const& name, std::string const& content) {
  std::ofstream file(fmt::format("{}.csv", name));
  file << content;
}

/////////////////////////////////////////////////
///////////////   TEST HARNESS    ///////////////
/////////////////////////////////////////////////

void PerfTestHarness::SetUp() {
  auto custom_argv = custom_args_.data();
  auto custom_argc = static_cast<int32_t>(custom_args_.size());
  CollectiveOps::initialize(custom_argc, custom_argv, no_workers, true);
  my_node_ = theContext()->getNode();
  num_nodes_ = theContext()->getNumNodes();
}

void PerfTestHarness::TearDown() {
  SpinScheduler();

  CollectiveOps::finalize();
  current_run_++;
}

void PerfTestHarness::Initialize(int argc, char** argv) {
  // pre-parsed args, that may contain performance-test specific args
  std::vector<char*> args_before(argv, argv + argc);

  for (auto& arg : args_before) {
    std::string arg_s{arg};
    if (arg_s == "--vt_perf_gen_file") {
      gen_file_ = true;
    } else if (arg_s == "--vt_perf_verbose") {
      verbose_ = true;
    } else if (arg_s.substr(0, 18) == "--vt_perf_num_runs") {
      // Assume it's in form '--vt_perf_num_runs={num}'
      num_runs_ = stoi(arg_s.substr(19));
    } else {
      custom_args_.push_back(arg);
    }
  }

  if(!verbose_){
    custom_args_.push_back(const_cast<char*>("--vt_quiet"));
  }

  memory_use_.resize(num_runs_);
  timings_.resize(num_runs_);

  // PerfTestHarness::Initialize is called before vt initializes
  vt::debug::preConfigRef()->colorize_output = true;
}

std::string PerfTestHarness::GetName() const {
  return name_;
}

uint32_t PerfTestHarness::GetNumRuns() const {
  return num_runs_;
}

std::string PerfTestHarness::OutputMemoryUse() const {
  std::string file_content =  "name,node,mem\n";

  fmt::print("\n{}----- MEMORY -----{}\n", debug::bold(), debug::reset());
  for (auto const& per_node_mem : combined_mem_use_) {
    auto const node = per_node_mem.first;
    auto const& memory_use = per_node_mem.second;

    std::size_t cur_min = std::numeric_limits<std::size_t>::max();
    std::size_t cur_max = 0;

    for (auto const mem : memory_use) {
      cur_min = std::min(mem.min_, cur_min);
      cur_max = std::max(mem.max_, cur_max);

      file_content.append(fmt::format("{},{},{}\n", name_, node, mem.mean_));

      if (verbose_) {
        fmt::print(
          "{} Memory usage: {}\n", debug::proc(node),
          GetFormattedMemUsage(mem.mean_));
      }
    }

    fmt::print(
      "{} {}: Memory usage: min: {} max: {}\n", debug::proc(node),
      debug::reg(name_), GetFormattedMemUsage(cur_min),
      GetFormattedMemUsage(cur_max)
    );
  }

  return file_content;
}

std::string PerfTestHarness::OutputTimeResults() {
  std::string file_content = "name,node,mean,stdev\n";

  auto HandleTestResults =
    [&file_content]
    (std::pair<std::string, PerNodeResults>& per_node_results) mutable {
      auto const name = per_node_results.first;

      for (auto& per_node_result : per_node_results.second) {
        auto const node = per_node_result.first;
        auto& timings = per_node_result.second;

        file_content.append(fmt::format(
          "{},{},{:.3f},{:.3f}\n", name, node, timings.mean_,
          timings.std_dev_)
        );

        fmt::print(
          "{} Results for {} (avg: {} stdev: {} min: {} max: {})\n",
          debug::proc(node), debug::reg(name),
          debug::emph(fmt::format("{:.3f}ms", timings.mean_)),
          debug::emph(fmt::format("{:.3f}ms", timings.std_dev_)),
          debug::emph(fmt::format("{:.3f}ms", timings.min_)),
          debug::emph(fmt::format("{:.3f}ms", timings.max_))
        );
      }
    };

  // First handle the main test result
  auto main_test = std::find_if(
    combined_timings_.begin(), combined_timings_.end(),
    [this](auto const& test_run) { return test_run.first == name_; }
  );

  if (main_test != combined_timings_.end()) {
    HandleTestResults(*main_test);
    combined_timings_.erase(main_test);
  }

  fmt::print("\n{}----- TIMERS -----{}\n", debug::bold(), debug::reset());
  for (auto& test_run : combined_timings_) {
    HandleTestResults(test_run);
  }

  return file_content;
}

void PerfTestHarness::DumpResults() {
  // Only dump results on root node
  if (my_node_ == 0) {
    fmt::print(
      "\nTest results for {} running on {} nodes:\n", debug::emph(name_),
      debug::emph(fmt::format("{}", num_nodes_))
    );

    auto const time_file_data = OutputTimeResults();
    auto const memory_file = OutputMemoryUse();

    if (gen_file_) {
      OutputToFile(fmt::format("{}_mem", name_), memory_file);
      OutputToFile(fmt::format("{}_time", name_), time_file_data);
    }
  }
}

void PerfTestHarness::AddResult(TestResult const& test_result) {
  timings_[current_run_].push_back(test_result);
}

void PerfTestHarness::StartTimer(std::string const& name) {
  timers_[name].Start();
}

void PerfTestHarness::StopTimer(std::string const& name) {
  AddResult({name, timers_[name].Stop()});
}

void PerfTestHarness::SyncResults() {
  auto proxy = theObjGroup()->makeCollective<TestNodeObj>(this);

  // Root node will be responsible for generating the final output
  // so every other node sends its results to it (at the end of test runs)
  runInEpochCollective([proxy, this] {
    constexpr auto root_node = 0;

    if (my_node_ != root_node) {
      proxy[root_node].send<TestMsg, &TestNodeObj::RecvTestResult>(
        timings_, memory_use_, my_node_);
    } else {
      // Copy the root node's data to combined structures
      CopyTestData(timings_, memory_use_, my_node_);
    }
  });
}

void PerfTestHarness::CopyTestData(
  PerfTestHarness::TestResults const& source_time,
  PerfTestHarness::MemoryUsage const& source_memory, NodeType const node
) {
  auto time_use =
    ProcessInput<PerfTestHarness::TestResult, PerfTestHarness::FinalTestResult>(
      source_time,
      [](
        PerfTestHarness::FinalTestResult& left,
        PerfTestHarness::TestResult const& right, std::size_t const num_elems) {
        if (left.first.empty()) {
          left = {
            right.first,
            {right.second / num_elems, 0, right.second, right.second}};
        } else {
          left.second.mean_ += right.second / num_elems;
          left.second.min_ = std::min(left.second.min_, right.second);
          left.second.max_ = std::max(left.second.max_, right.second);
        }
      },
      [](
        PerfTestHarness::FinalTestResult& left,
        PerfTestHarness::TestResult const& right, std::size_t const num_elems,
        bool const is_last) {
        left.second.std_dev_ +=
          std::pow(right.second - left.second.mean_, 2) / num_elems;

        if (is_last) {
          left.second.std_dev_ = std::sqrt(left.second.std_dev_);
        }
      });

  for (auto const& test_result : time_use) {
    auto const& test_name = test_result.first;
    auto const time = test_result.second;

    auto const& it = std::find_if(
      combined_timings_.begin(), combined_timings_.end(),
      [test_name](auto const& result) { return test_name == result.first; }
    );

    if (it != combined_timings_.end()) {
      it->second[node] = time;
    } else {
      PerfTestHarness::PerNodeResults map;
      map[node] = time;

      combined_timings_.push_back({test_name, map});
    }
  }

  auto mem_use = ProcessInput<size_t, TestResultHolder<size_t>>(
    source_memory,
    [](
      TestResultHolder<size_t>& left, size_t const& right,
      std::size_t const num_elems
      )
    {
      left.mean_ += right / num_elems;
      left.min_ = std::min(left.min_, right);
      left.max_ = std::max(left.max_, right);
    },
    [](
      TestResultHolder<size_t>& left, size_t const& right,
      std::size_t const num_elems, bool const is_last
      )
    {
      left.std_dev_ += std::pow(right - left.mean_, 2) / num_elems;

      if (is_last) {
        left.std_dev_ = std::sqrt(left.std_dev_);
      }
    }
  );

  combined_mem_use_[node].resize(mem_use.size());

  std::copy(mem_use.begin(), mem_use.end(), combined_mem_use_[node].begin());
}

void PerfTestHarness::SpinScheduler() {
  theSched()->runSchedulerWhile([] { return !rt->isTerminated(); });
}

void PerfTestHarness::GetMemoryUsage() {
  // Memory footpring from PerfTestHarness' internal data structs are included
  memory_use_[current_run_].push_back(mem_tracker_.getUsage());
}

}}}} // namespace vt::tests::perf::common
