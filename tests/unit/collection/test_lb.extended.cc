/*
//@HEADER
// *****************************************************************************
//
//                             test_lb.extended.cc
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
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/vrt/collection/manager.h"
#include "vt/utils/json/json_reader.h"

#include <nlohmann/json.hpp>

#include <dirent.h>

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit { namespace lb {

static constexpr int const num_elms = 64;
static constexpr int const num_phases = 10;

struct MyCol : vt::Collection<MyCol,vt::Index1D> {
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::Collection<MyCol,vt::Index1D>::serialize(s);
    s | val;
  }

  double val = 0.0;
};

using MyMsg = vt::CollectionMessage<MyCol>;

// A dummy kernel that does some work depending on the index
void colHandler(MyMsg*, MyCol* col) {
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < col->getIndex().x() * 20; j++) {
      col->val += (i*29+j*2)-4;
    }
  }
}

struct TestLoadBalancer : TestParallelHarnessParam<std::string> {
  void runTest();
};

void TestLoadBalancer::runTest() {
  auto lb_name = GetParam();

  vt::theConfig()->vt_lb = true;
  vt::theConfig()->vt_lb_name = lb_name;
  if (vt::theContext()->getNode() == 0) {
    fmt::print("Testing lb {}\n", lb_name);
  }
  if (lb_name.compare("TemperedLB") == 0) {
    std::string lb_args("ordering=Arbitrary rollback=false");
    vt::theConfig()->vt_lb_args = lb_args;
    if (vt::theContext()->getNode() == 0) {
      fmt::print("Using lb_args {}\n", lb_args);
    }
  }
  if (lb_name.substr(0, 8).compare("GreedyLB") == 0) {
    vt::theConfig()->vt_lb_name = "GreedyLB";
    auto strat_arg = lb_name.substr(9, lb_name.size() - 9);
    fmt::print("strat_arg={}\n", strat_arg);
    vt::theConfig()->vt_lb_args = strat_arg;
  }

  vt::theCollective()->barrier();

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<MyCol> proxy;

  // Construct a collection
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<MyCol>(range);
  });

  for (int phase = 0; phase < num_phases; phase++) {
    // Do some work.
    runInEpochCollective([&]{
      proxy.broadcastCollective<MyMsg, colHandler>();
    });

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }
  return;
}

TEST_P(TestLoadBalancer, test_load_balancer_1) {
  runTest();
}

TEST_P(TestLoadBalancer, test_load_balancer_keep_last_elm) {
  vt::theConfig()->vt_lb_keep_last_elm = true;
  runTest();
}

struct MyCol2 : vt::Collection<MyCol2,vt::Index1D> {};

TEST_P(TestLoadBalancer, test_load_balancer_no_work) {
  auto const num_nodes = theContext()->getNumNodes();
  auto const range = Index1D(num_nodes * 8);
  theCollection()->constructCollective<MyCol2>(
    range, [](vt::Index1D) { return std::make_unique<MyCol2>(); }
  );

  vt::theConfig()->vt_lb = true;
  vt::theConfig()->vt_lb_name = "RotateLB";
  vt::theConfig()->vt_lb_interval = 1;

  vt::theCollective()->barrier();

  for (int i = 0; i < 10; i++) {
    vt::runInEpochCollective([&]{
      vt::thePhase()->nextPhaseCollective();
    });
  }
}

auto balancers = ::testing::Values(
    "RandomLB",
    "RotateLB",
    "HierarchicalLB",
    "TemperedLB",
    "GreedyLB:data_dist=scatter",
    "GreedyLB:data_dist=pt2pt",
    "GreedyLB:data_dist=bcast"
#   if vt_check_enabled(zoltan)
    , "ZoltanLB"
#   endif
);

INSTANTIATE_TEST_SUITE_P(
  LoadBalancerExplode, TestLoadBalancer, balancers
);

struct TestParallelHarnessWithStatsDumping : TestParallelHarnessParam<int> {
  virtual void addAdditionalArgs() override {
    static char vt_lb_stats[]{"--vt_lb_stats"};
    static char vt_lb_stats_dir[]{"--vt_lb_stats_dir=test_stats_dir"};
    static char vt_lb_stats_file[]{"--vt_lb_stats_file=test_stats_outfile.%p.json"};

    addArgs(vt_lb_stats, vt_lb_stats_dir, vt_lb_stats_file);
  }
};

struct TestNodeStatsDumper : TestParallelHarnessWithStatsDumping {};

void closeNodeStatsFile(char const* file_path);
int countCreatedStatsFiles(char const* path);
void removeStatsOutputDir(char const* path);
std::map<int, int> getPhasesFromStatsFile(const char* file_path);

TEST_P(TestNodeStatsDumper, test_node_stats_dumping_with_interval) {
  vt::theConfig()->vt_lb = true;
  vt::theConfig()->vt_lb_name = "GreedyLB";
  vt::theConfig()->vt_lb_interval = GetParam();

  if (vt::theContext()->getNode() == 0) {
    fmt::print(
      "Testing dumping Node Stats with LB interval {}\n",
      vt::theConfig()->vt_lb_interval
    );
  }

  vt::vrt::collection::CollectionProxy<MyCol> proxy;
  auto const range = vt::Index1D(num_elms);

  // Construct a collection
  runInEpochCollective([&] {
    proxy = vt::theCollection()->constructCollective<MyCol>(range);
  });

  for (int phase = 0; phase < num_phases; phase++) {
    // Do some work
    runInEpochCollective([&] {
      proxy.broadcastCollective<MyMsg, colHandler>();
    });

    // Go to the next phase
    vt::thePhase()->nextPhaseCollective();
  }

  // Finalize to get data output
  theNodeStats()->finalize();

  using vt::util::json::Reader;

  vt::runInEpochCollective([=]{
    Reader r(theConfig()->getLBStatsFileOut());
    auto json_ptr = r.readFile();
    auto& json = *json_ptr;

    EXPECT_TRUE(json.find("phases") != json.end());
    EXPECT_EQ(json["phases"].size(), num_phases);
  });

  if (vt::theContext()->getNode() == 0) {
    removeStatsOutputDir(vt::theConfig()->vt_lb_stats_dir.c_str());
  }

  // Prevent NodeStats from closing files during finalize()
  // All the tmp files are removed already
  vt::theConfig()->vt_lb_stats = false;
}

int countCreatedStatsFiles(char const* path) {
  int files_counter = 0;
  if (auto* dir = opendir(path)) {
    while (auto* dir_ent = readdir(dir)) {
      if (
        strcmp(dir_ent->d_name, ".") == 0 ||
        strcmp(dir_ent->d_name, "..") == 0
      ) {
        continue;
      }

      std::string file_path = std::string{path} + '/' + dir_ent->d_name;
      struct stat stat_buf;
      if (stat(file_path.c_str(), &stat_buf) == 0 && stat_buf.st_size > 0) {
        ++files_counter;
      }
    }

    closedir(dir);
  }

  return files_counter;
}

void removeStatsOutputDir(char const* path) {
  if (auto* dir = opendir(path)) {
    while (auto* dir_ent = readdir(dir)) {
      if (
        strcmp(dir_ent->d_name, ".") == 0 ||
        strcmp(dir_ent->d_name, "..") == 0
      ) {
        continue;
      }

      std::string file_path = std::string{path} + '/' + dir_ent->d_name;
      auto const* path_cstr = file_path.c_str();
      unlink(path_cstr);
    }

    rmdir(path);
  }
}

auto const intervals = ::testing::Values(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

INSTANTIATE_TEST_SUITE_P(
  NodeStatsDumperExplode, TestNodeStatsDumper, intervals
);

}}}} // end namespace vt::tests::unit::lb

#endif /*vt_check_enabled(lblite)*/
