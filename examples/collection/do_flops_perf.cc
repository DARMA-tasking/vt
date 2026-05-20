/*
//@HEADER
// *****************************************************************************
//
//                               do_flops_perf.cc
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

/// [Do Flops example]

#include <vt/transport.h>
#include <vt/runnable/invoke.h>

#include <algorithm>
#include <cstdlib>

static constexpr std::size_t const default_num_objs = 100;
static constexpr std::size_t const default_flops_per_iter = 100000;

double pi(uint64_t n) {
  double sum = 0.0;
  int sign = 1;
  for (uint64_t i = 0; i < n; ++i) {
    sum += sign/(2.0*i+1.0);
    sign *= -1;
  }
  return 4.0*sum;
}

void printMultiplexingInfo(
  std::vector<vt::metrics::PerfData::TaskGroupMeasurements> const& groups
) {
  for (auto const& group : groups) {
    fmt::print(
      "  group {}: time_enabled={}, time_running={}, enabled/running={:.6Lf}, running/enabled={:.6Lf}\n",
      group.group_.group_name_,
      group.time_enabled_,
      group.time_running_,
      group.getScalingRatio(),
      group.getRunningFraction()
    );
  }
}

struct GenericWork : vt::Collection<GenericWork, vt::Index1D> {
  void doIteration() {
    fmt::print("-- Starting Iteration --\n");

    vt::theContext()->getTask()->startMetrics();

    // ----------------------------------------------------------
    // test non packed double precision floating point operations
    // should result in ~4*n of these operations
    double p = pi(flopsPerIter_);
    fmt::print("pi: {}\n", p);
    // ----------------------------------------------------------

    vt::theContext()->getTask()->stopMetrics();
    auto const group_measurements = vt::thePerfData()->getTaskGroupMeasurements();
    std::unordered_map<std::string, uint64_t> res;
    for (auto const& group : group_measurements) {
      for (auto const& measurement : group.measurements_) {
        res[measurement.first] = measurement.second;
      }
    }

    auto const expected_ops = 4ull * static_cast<uint64_t>(flopsPerIter_);
    fmt::print("  expected scalar fp ops ~= {}\n", expected_ops);
    printMultiplexingInfo(group_measurements);

    std::vector<std::pair<std::string, uint64_t>> ordered(res.begin(), res.end());
    std::sort(ordered.begin(), ordered.end());

    for (auto const& [name, value] : ordered) {
      fmt::print("  {}: {}", name, value);
      if (expected_ops > 0) {
        auto const ratio = static_cast<long double>(value) /
          static_cast<long double>(expected_ops);
        fmt::print(" (ratio to expected ops = {:.6Lf})", ratio);
      }
      fmt::print("\n");
    }

    fmt::print("-- Stopping Iteration --\n");
  }

  void init(int in_flops_per_iter) {
    flopsPerIter_ = in_flops_per_iter;
  }

private:
  size_t flopsPerIter_ = 0;
};

int main(int argc, char** argv) {
  size_t num_objs = default_num_objs;
  size_t flopsPerIter = default_flops_per_iter;
  size_t maxIter = 8;

  std::string name(argv[0]);

  vt::initialize(argc, argv);

  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (argc == 1) {
    if (vt::theContext()->getNode() == 0) {
      fmt::print(stderr, "{}: using default arguments since none provided\n", name);
    }
    num_objs = default_num_objs * num_nodes;
  } else if (argc == 2) {
    num_objs = static_cast<size_t>(strtol(argv[1], nullptr, 10));
  } else if (argc == 3) {
    num_objs = static_cast<size_t>(strtol(argv[1], nullptr, 10));
    flopsPerIter = static_cast<size_t>(strtol(argv[2], nullptr, 10));
  } else if (argc == 4) {
    num_objs = static_cast<size_t>(strtol(argv[1], nullptr, 10));
    flopsPerIter = static_cast<size_t>(strtol(argv[2], nullptr, 10));
    maxIter = static_cast<size_t>(strtol(argv[3], nullptr, 10));
  } else {
    fmt::print(stderr, "usage: {} <num-objects> <flops-per-iter> <maxiter>\n", name);
    return 1;
  }

  using BaseIndexType = typename vt::Index1D::DenseIndexType;
  auto range = vt::Index1D(static_cast<BaseIndexType>(num_objs));

  auto col_proxy = vt::makeCollection<GenericWork>("examples_generic_work")
    .bounds(range)
    .bulkInsert()
    .wait();

  vt::runInEpochCollective([&]{
    col_proxy.broadcastCollective<&GenericWork::init>(flopsPerIter);
  });

  for (std::size_t i = 0; i < maxIter; i++) {
    vt::runInEpochCollective([&]{
      col_proxy.broadcastCollective<&GenericWork::doIteration>();
    });
    vt::thePhase()->nextPhaseCollective();
  }

  vt::finalize();

  return 0;
}
/// [Do Flops example]
