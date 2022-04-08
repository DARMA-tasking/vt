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
#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/utils/json/json_reader.h"
#include "vt/utils/json/json_appender.h"

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

struct TestLoadBalancerOther : TestParallelHarnessParam<std::string> { };
struct TestLoadBalancerGreedy : TestParallelHarnessParam<std::string> { };
struct TestLoadBalancerCharm : TestParallelHarnessParam<std::string> { };

void runTest(std::string lb_name) {
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
  if (lb_name.substr(0, 7).compare("CharmLB") == 0) {
    vt::theConfig()->vt_lb_name = "CharmLB";
    auto strat_arg = lb_name.substr(8, lb_name.size() - 8);
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

TEST_P(TestLoadBalancerOther, test_load_balancer_other_1) {
  runTest(GetParam());
}

TEST_P(TestLoadBalancerOther, test_load_balancer_other_keep_last_elm) {
  vt::theConfig()->vt_lb_keep_last_elm = true;
  runTest(GetParam());
}

TEST_P(TestLoadBalancerGreedy, test_load_balancer_greedy_2) {
  runTest(GetParam());
}

TEST_P(TestLoadBalancerGreedy, test_load_balancer_greedy_keep_last_elm) {
  vt::theConfig()->vt_lb_keep_last_elm = true;
  runTest(GetParam());
}

TEST_P(TestLoadBalancerCharm, test_load_balancer_charm_2) {
  runTest(GetParam());
}

TEST_P(TestLoadBalancerCharm, test_load_balancer_charm_keep_last_elm) {
  vt::theConfig()->vt_lb_keep_last_elm = true;
  runTest(GetParam());
}

struct MyCol2 : vt::Collection<MyCol2,vt::Index1D> {};

using TestLoadBalancerNoWork = TestParallelHarness;

TEST_F(TestLoadBalancerNoWork, test_load_balancer_no_work) {
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

auto balancers_other = ::testing::Values(
    "RandomLB",
    "RotateLB",
    "HierarchicalLB",
    "TemperedLB"
#   if vt_check_enabled(zoltan)
    , "ZoltanLB"
#   endif
);

auto balancers_greedy = ::testing::Values(
    "GreedyLB:strategy=scatter",
    "GreedyLB:strategy=pt2pt",
    "GreedyLB:strategy=bcast"
);

auto balancers_charm = ::testing::Values(
    "CharmLB:strategy=scatter",
    "CharmLB:strategy=pt2pt",
    "CharmLB:strategy=bcast"
);


INSTANTIATE_TEST_SUITE_P(
  LoadBalancerExplodeOther, TestLoadBalancerOther, balancers_other
);

INSTANTIATE_TEST_SUITE_P(
  LoadBalancerExplodeGreedy, TestLoadBalancerGreedy, balancers_greedy
);

INSTANTIATE_TEST_SUITE_P(
  LoadBalancerExplodeCharm, TestLoadBalancerCharm, balancers_charm
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

using TestRestoreStatsData = TestParallelHarness;

vt::vrt::collection::balance::StatsData
getStatsDataForPhase(
  vt::PhaseType phase,  vt::vrt::collection::balance::StatsData in
) {
  using JSONAppender = vt::util::json::Appender<std::stringstream>;
  using vt::vrt::collection::balance::StatsData;
  using json = nlohmann::json;
  std::stringstream ss{std::ios_base::out | std::ios_base::in};
  auto ap = std::make_unique<JSONAppender>("phases", std::move(ss), false);
  auto j = in.toJson(phase);
  ap->addElm(*j);
  ss = ap->finish();
  //fmt::print("{}\n", ss.str());
  return StatsData{json::parse(ss)};
}

TEST_F(TestRestoreStatsData, test_restore_stats_data_1) {
  auto this_node = vt::theContext()->getNode();
  std::string out_file_name = "test_restore_stats_data_1.%p.json";
  std::size_t rank = out_file_name.find("%p");
  auto str_rank = std::to_string(this_node);
  if (rank == std::string::npos) {
    out_file_name = out_file_name + str_rank;
  } else {
    out_file_name.replace(rank, 2, str_rank);
  }

  vt::vrt::collection::CollectionProxy<MyCol> proxy;
  auto const range = vt::Index1D(num_elms);

  // Construct a collection
  runInEpochCollective([&] {
    proxy = vt::theCollection()->constructCollective<MyCol>(range);
  });

  vt::vrt::collection::balance::StatsData sd;
  PhaseType write_phase = 0;

  using CommKey = vt::elm::CommKey;
  using CommVolume = vt::elm::CommVolume;
  using CommBytesType = vt::elm::CommBytesType;

  // @todo: should do other types of comm

  {
    PhaseType phase = write_phase;
    sd.node_data_[phase];
    sd.node_comm_[phase];

    for (int i=0; i<num_elms; ++i) {
      vt::Index1D idx(i);

      TimeType dur = (i % 10 + 1) * 0.1;
      uint64_t ntocm = (i+1) % 3 + 2;
      CommBytesType ntoc = (i+1) * 100;
      uint64_t ctonm = (i+1) % 2 + 1;
      CommBytesType cton = (i+1) * 200;

      auto elm_ptr = proxy(idx).tryGetLocalPtr();
      if (elm_ptr != nullptr) {
        auto elm_id = elm_ptr->getElmID();

        std::vector<TimeType> dur_vec(2);
        dur_vec[i % 2] = dur;
        sd.node_data_[phase][elm_id].whole_phase_load = dur;
        sd.node_data_[phase][elm_id].subphase_loads = dur_vec;

        CommKey ntockey(
          CommKey::NodeToCollectionTag{}, this_node, elm_id, false
        );
        CommVolume ntocvol{ntoc, ntocm};
        sd.node_comm_[phase][ntockey] = ntocvol;
        sd.node_subphase_comm_[phase][i % 2][ntockey] = ntocvol;

        CommKey ctonkey(
          CommKey::CollectionToNodeTag{}, elm_id, this_node, false
        );
        CommVolume ctonvol{cton, ctonm};
        sd.node_comm_[phase][ctonkey] = ctonvol;
        sd.node_subphase_comm_[phase][(i + 1) % 2][ctonkey] = ctonvol;

        std::vector<uint64_t> arr;
        arr.push_back(idx.x());
        sd.node_idx_[elm_id] = std::make_tuple(proxy.getProxy(), arr);
      }
    }
  }

  auto sd_read = getStatsDataForPhase(write_phase, sd);

  // whole-phase loads
  EXPECT_EQ(sd_read.node_data_.size(), sd.node_data_.size());
  if (sd_read.node_data_.size() != sd.node_data_.size()) {
    fmt::print(
      "Wrote {} phases of whole-phase load data but read in {} phases",
      sd.node_data_.size(), sd_read.node_data_.size()
    );
  } else {
    // compare the whole-phase load data in detail
    for (auto &phase_data : sd.node_data_) {
      auto phase = phase_data.first;
      EXPECT_FALSE(sd_read.node_data_.find(phase) == sd_read.node_data_.end());
      if (sd_read.node_data_.find(phase) == sd_read.node_data_.end()) {
        fmt::print(
          "Phase {} in whole-phase loads were not read in",
          phase
        );
      } else {
        auto &read_load_map = sd_read.node_data_[phase];
        auto &orig_load_map = phase_data.second;
        for (auto &entry : read_load_map) {
          auto read_elm_id = entry.first;
          EXPECT_FALSE(orig_load_map.find(read_elm_id) == orig_load_map.end());
          if (orig_load_map.find(read_elm_id) == orig_load_map.end()) {
            fmt::print(
              "Unexpected element ID read in whole-phase loads on phase={}: "
              "id={}, home={}, curr={}",
              phase,
              read_elm_id.id, read_elm_id.getHomeNode(), read_elm_id.curr_node
            );
          } else {
            auto orig_elm_id = orig_load_map.find(read_elm_id)->first;
            EXPECT_EQ(read_elm_id.getHomeNode(), orig_elm_id.getHomeNode());
            EXPECT_EQ(read_elm_id.curr_node, orig_elm_id.curr_node);
            if (
              read_elm_id.getHomeNode() != orig_elm_id.getHomeNode() ||
              read_elm_id.curr_node != orig_elm_id.curr_node
            ) {
              fmt::print(
                "Corrupted element ID read in whole-phase loads on phase={}: "
                "id={}, home={}, curr={} (expected id={}, home={}, curr={})",
                phase,
                read_elm_id.id, read_elm_id.getHomeNode(), read_elm_id.curr_node,
                orig_elm_id.id, orig_elm_id.getHomeNode(), orig_elm_id.curr_node
              );
            }
            auto read_load = read_load_map[read_elm_id];
            auto orig_load = entry.second;
            // @todo: make this a more robust floating point comparison
            EXPECT_EQ(orig_load.whole_phase_load, read_load.whole_phase_load);
            EXPECT_EQ(orig_load.subphase_loads, read_load.subphase_loads);
          }
        }
      }
    }
  }

  // element id to index mapping
  EXPECT_EQ(sd_read.node_idx_.size(), sd.node_idx_.size());
  if (sd_read.node_idx_.size() != sd.node_idx_.size()) {
    fmt::print(
      "Wrote index mapping for {} elements but read in {}",
      sd.node_idx_.size(), sd_read.node_idx_.size()
    );
  } else {
    // detailed comparison of element id to index mapping
    for (auto &entry : sd_read.node_idx_) {
      auto read_elm_id = entry.first;
      EXPECT_FALSE(sd.node_idx_.find(read_elm_id) == sd.node_idx_.end());
      if (sd.node_idx_.find(read_elm_id) == sd.node_idx_.end()) {
        fmt::print(
          "Unexpected element ID read in index mapping: "
          "id={}, home={}, curr={}",
          read_elm_id.id, read_elm_id.getHomeNode(), read_elm_id.curr_node
        );
      } else {
        auto orig_idx = sd.node_idx_[read_elm_id];
        auto read_idx = entry.second;
        EXPECT_EQ(orig_idx, read_idx);
        if (orig_idx != read_idx) {
          fmt::print(
            "Unexpected collection index for elm id={}, home={}, curr={}",
            read_elm_id.id, read_elm_id.getHomeNode(), read_elm_id.curr_node
          );
        }
      }
    }
  }

  // whole-phase communication
  EXPECT_EQ(sd_read.node_comm_.size(), sd.node_comm_.size());
  if (sd_read.node_comm_.size() != sd.node_comm_.size()) {
    fmt::print(
      "Wrote {} phases of whole-phase comm data but read in {} phases",
      sd.node_comm_.size(), sd_read.node_comm_.size()
    );
  }
  // @todo: detailed comparison of whole-phase comm data

  // @todo: compare subphase comm when writing/reading is implemented
  // @todo: detailed comparison of subphase comm data

  // @todo: clean up files
}

}}}} // end namespace vt::tests::unit::lb

#endif /*vt_check_enabled(lblite)*/
