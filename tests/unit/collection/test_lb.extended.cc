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

#include "nlohmann/detail/meta/is_sax.hpp"
#include "nlohmann/json_fwd.hpp"
#include "test_parallel_harness.h"
#include "test_helpers.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/phase/phase_manager.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/balance/lb_data_holder.h"
#include "vt/vrt/collection/balance/node_lb_data.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/temperedwmin/temperedwmin.h"
#include "vt/utils/json/json_reader.h"
#include "vt/utils/json/json_appender.h"
#include "vt/utils/file_spec/spec.h"

#include <nlohmann/json.hpp>
#include <memory>
#include <sstream>

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

// A dummy kernel that does some work depending on the index
void colHandler(MyCol* col) {
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < col->getIndex().x() * 20; j++) {
      col->val += (i*29+j*2)-4;
    }
  }
}

struct TestLoadBalancerOther : TestParallelHarnessParam<std::string> { };
struct TestLoadBalancerGreedy : TestParallelHarnessParam<std::string> { };

void runTest(std::string const& lb_name, std::string const& label) {
  vt::theConfig()->vt_lb = true;
  vt::theConfig()->vt_lb_name = lb_name;
  if (vt::theContext()->getNode() == 0) {
    fmt::print("Testing lb {}\n", lb_name);
  }
  if (
    lb_name.compare("TemperedLB") == 0 ||
    lb_name.compare("TemperedWMin") == 0
  ) {
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
    proxy = vt::theCollection()->constructCollective<MyCol>(range, label);
  });

  for (int phase = 0; phase < num_phases; phase++) {
    // Do some work.
    runInEpochCollective([&]{
      proxy.broadcastCollective<colHandler>();
    });

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }
  return;
}

TEST_P(TestLoadBalancerOther, test_load_balancer_other_1) {
  runTest(GetParam(), "test_load_balancer_other_1");
}

TEST_P(TestLoadBalancerOther, test_load_balancer_other_keep_last_elm) {
  vt::theConfig()->vt_lb_keep_last_elm = true;
  runTest(GetParam(), "test_load_balancer_other_keep_last_elm");
}

TEST_P(TestLoadBalancerGreedy, test_load_balancer_greedy_2) {
  runTest(GetParam(), "test_load_balancer_greedy_2");
}

TEST_P(TestLoadBalancerGreedy, test_load_balancer_greedy_keep_last_elm) {
  vt::theConfig()->vt_lb_keep_last_elm = true;
  runTest(GetParam(), "test_load_balancer_greedy_keep_last_elm");
}

TEST_F(TestLoadBalancerOther, test_make_graph_symmetric) {
  // setup
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  auto const next_node = (this_node + 1) % num_nodes;

  auto id_from =
    elm::ElmIDBits::createCollectionImpl(true, 1, this_node, this_node);
  auto id_to =
    elm::ElmIDBits::createCollectionImpl(true, 2, next_node, next_node);

  elm::ElementLBData elm_data;
  double const bytes = 10.0;
  elm_data.sendToEntity(id_to, id_from, bytes);
  theNodeLBData()->addNodeLBData(id_from, &elm_data, nullptr);

  auto const phase = thePhase()->getCurrentPhase();
  auto const comm_data = theNodeLBData()->getNodeComm(phase);
  ASSERT_NE(comm_data, nullptr);
  ASSERT_EQ(comm_data->size(), 1);

  // test
  auto proxy = theLBManager()->makeLB<vt::vrt::collection::lb::TemperedWMin>();
  runInEpochCollective(
    "test_make_graph_symmetric -> makeGraphSymmetric",
    [phase, proxy] { vrt::collection::balance::makeGraphSymmetric(phase, proxy); }
  );
  vt::theLBManager()->destroyLB();

  // assert
  if (num_nodes == 1) {
    ASSERT_EQ(comm_data->size(), 1);
    return;
  }

  ASSERT_EQ(comm_data->size(), 2);
  auto const prev_node = (this_node + num_nodes - 1) % num_nodes;
  bool this_to_next = false, prev_to_this = false;

  for (auto&& elm : *comm_data) {
    auto const& comm_key = elm.first;
    auto const& comm_vol = elm.second;
    auto const from_home_node = comm_key.fromObj().getHomeNode();
    auto const to_home_node = comm_key.toObj().getHomeNode();

    if (from_home_node == this_node) {
      ASSERT_EQ(to_home_node, next_node);
      this_to_next = true;
    } else if (from_home_node == prev_node) {
      ASSERT_EQ(to_home_node, this_node);
      prev_to_this = true;
    }
    ASSERT_EQ(comm_vol.bytes, bytes);

    vt_debug_print(
      verbose, temperedwmin, "test_make_graph_symmetric: elm: from={}, to={}\n",
      comm_key.fromObj(), comm_key.toObj()
    );
  }

  // make sure that both (distinct) comms are present
  ASSERT_TRUE(this_to_next);
  ASSERT_TRUE(prev_to_this);
}

struct MyCol2 : vt::Collection<MyCol2,vt::Index1D> {};

using TestLoadBalancerNoWork = TestParallelHarness;

TEST_F(TestLoadBalancerNoWork, test_load_balancer_no_work) {
  auto const num_nodes = theContext()->getNumNodes();
  auto const range = Index1D(num_nodes * 8);
  theCollection()->constructCollective<MyCol2>(
    range, [](vt::Index1D) { return std::make_unique<MyCol2>(); },
    "test_load_balancer_no_work"
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
  "TemperedLB",
  "TemperedWMin"
#   if vt_check_enabled(zoltan)
  , "ZoltanLB"
#   endif
);

auto balancers_greedy = ::testing::Values(
    "GreedyLB:strategy=scatter",
    "GreedyLB:strategy=pt2pt",
    "GreedyLB:strategy=bcast"
);

INSTANTIATE_TEST_SUITE_P(
  LoadBalancerExplodeOther, TestLoadBalancerOther, balancers_other
);

INSTANTIATE_TEST_SUITE_P(
  LoadBalancerExplodeGreedy, TestLoadBalancerGreedy, balancers_greedy
);

struct TestParallelHarnessWithLBDataDumping : TestParallelHarnessParam<int> {
  virtual void addAdditionalArgs() override {
    static char vt_lb_data[]{"--vt_lb_data"};

    std::string lb_dir_file(getUniqueFilename("_dir"));
    std::string lb_dir_flag("--vt_lb_data_dir=");
    std::string lb_dir_arg = lb_dir_flag + lb_dir_file;
    char *vt_lb_data_dir = strdup(lb_dir_arg.c_str());

    std::stringstream ss;
    ss << "--vt_lb_data_file=test_data_outfile";
    int init = 0;
    MPI_Initialized(&init);
    if (init) {
      int num_ranks = 0;
      MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
      ss << "_" << num_ranks;
    }
    ss << ".%p.json";
    char *vt_lb_data_file = strdup(ss.str().c_str());

    addArgs(vt_lb_data, vt_lb_data_dir, vt_lb_data_file);
  }
};

struct TestNodeLBDataDumper : TestParallelHarnessWithLBDataDumping {};
struct TestLBSpecFile : TestParallelHarnessWithLBDataDumping {};

void closeNodeLBDataFile(char const* file_path);
int countCreatedLBDataFiles(char const* path);
void removeLBDataOutputDir(char const* path);
std::map<int, int> getPhasesFromLBDataFile(const char* file_path);

TEST_P(TestLBSpecFile, test_node_lb_data_dumping_with_spec_file) {
  using namespace ::vt::utils::file_spec;

  vt::theConfig()->vt_lb = true;
  vt::theConfig()->vt_lb_name = "GreedyLB";

  std::string const file_name(getUniqueFilenameWithRanks(".txt"));
  if (theContext()->getNode() == 0) {
    std::ofstream out(file_name);
    out << ""
      "0 0 2\n"
      "%5 -1 1\n";
    out.close();
  }
  theCollective()->barrier();

  theConfig()->vt_lb_spec = true;
  theConfig()->vt_lb_spec_file = file_name;
  theNodeLBData()->loadAndBroadcastSpec();

  vt::vrt::collection::CollectionProxy<MyCol> proxy;
  auto const range = vt::Index1D(num_elms);

  // Construct a collection
  runInEpochCollective([&] {
    proxy = vt::theCollection()->constructCollective<MyCol>(
      range, "test_node_lb_data_dumping_with_spec_file"
    );
  });

  for (int phase = 0; phase < num_phases; phase++) {
    // Do some work
    runInEpochCollective([&] {
      proxy.broadcastCollective<colHandler>();
    });

    // Go to the next phase
    vt::thePhase()->nextPhaseCollective();
  }

  // Finalize to get data output
  theNodeLBData()->finalize();

  using vt::util::json::Reader;

  vt::runInEpochCollective([=]{
    Reader r(theConfig()->getLBDataFileOut());
    auto json_ptr = r.readFile();
    auto& json = *json_ptr;

    EXPECT_TRUE(json.find("phases") != json.end());

    // Expected phases for given spec file
    // All phases except 3, 7 and 8 should be added to json
    nlohmann::json expected_phases = R"(
    [{"id":0}, {"id":1}, {"id":2}, {"id":4}, {"id":5}, {"id":6}, {"id":9}])"_json;

    EXPECT_EQ(json["phases"].size(), expected_phases.size());

    for (decltype(expected_phases.size()) i = 0; i < expected_phases.size(); ++i) {
      EXPECT_EQ(expected_phases.at(i)["id"], json["phases"].at(i)["id"]);
    }
  });

  if (vt::theContext()->getNode() == 0) {
    removeLBDataOutputDir(vt::theConfig()->vt_lb_data_dir.c_str());
  }

  // Prevent NodeLBData from closing files during finalize()
  // All the tmp files are removed already
  vt::theConfig()->vt_lb_data = false;
}

TEST_P(TestNodeLBDataDumper, test_node_lb_data_dumping_with_interval) {
  vt::theConfig()->vt_lb = true;
  vt::theConfig()->vt_lb_name = "GreedyLB";
  vt::theConfig()->vt_lb_interval = GetParam();

  if (vt::theContext()->getNode() == 0) {
    fmt::print(
      "Testing dumping Node LB data with LB interval {}\n",
      vt::theConfig()->vt_lb_interval
    );
  }

  vt::vrt::collection::CollectionProxy<MyCol> proxy;
  auto const range = vt::Index1D(num_elms);

  // Construct a collection
  runInEpochCollective([&] {
    proxy = vt::theCollection()->constructCollective<MyCol>(
      range, "test_node_stats_dumping_with_interval"
    );
  });

  for (int phase = 0; phase < num_phases; phase++) {
    // Do some work
    runInEpochCollective([&] {
      proxy.broadcastCollective<colHandler>();
    });

    // Go to the next phase
    vt::thePhase()->nextPhaseCollective();
  }

  // Finalize to get data output
  theNodeLBData()->finalize();

  using vt::util::json::Reader;

  vt::runInEpochCollective([=]{
    Reader r(theConfig()->getLBDataFileOut());
    auto json_ptr = r.readFile();
    auto& json = *json_ptr;

    EXPECT_TRUE(json.find("phases") != json.end());
    EXPECT_EQ(json["phases"].size(), num_phases);
  });

  if (vt::theContext()->getNode() == 0) {
    removeLBDataOutputDir(vt::theConfig()->vt_lb_data_dir.c_str());
  }

  // Prevent NodeLBData from closing files during finalize()
  // All the tmp files are removed already
  vt::theConfig()->vt_lb_data = false;
}

int countCreatedLBDataFiles(char const* path) {
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

void removeLBDataOutputDir(char const* path) {
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
  NodeLBDataDumperExplode, TestNodeLBDataDumper, intervals
);

INSTANTIATE_TEST_SUITE_P(
  LBSpecFile, TestLBSpecFile, ::testing::Values(0)
);

using TestRestoreLBData = TestParallelHarness;

vt::vrt::collection::balance::LBDataHolder
getLBDataForPhase(
  vt::PhaseType phase,  vt::vrt::collection::balance::LBDataHolder in
) {
  using JSONAppender = vt::util::json::Appender<std::stringstream>;
  using vt::vrt::collection::balance::LBDataHolder;
  using json = nlohmann::json;
  std::stringstream ss{std::ios_base::out | std::ios_base::in};
  nlohmann::json metadata;
  metadata["type"] = "LBDatafile";
  auto ap = std::make_unique<JSONAppender>(
    "phases", metadata, std::move(ss), false
  );
  auto j = in.toJson(phase);
  ap->addElm(*j);
  ss = ap->finish();
  //fmt::print("{}\n", ss.str());
  return LBDataHolder{json::parse(ss)};
}

TEST_F(TestRestoreLBData, test_restore_lb_data_data_1) {
  auto this_node = vt::theContext()->getNode();

  vt::vrt::collection::CollectionProxy<MyCol> proxy;
  auto const range = vt::Index1D(num_elms);

  // Construct a collection
  runInEpochCollective([&] {
    proxy = vt::theCollection()->constructCollective<MyCol>(
      range, "test_restore_stats_data_1"
    );
  });

  vt::vrt::collection::balance::LBDataHolder lbdh;
  PhaseType write_phase = 0;

  using CommKey = vt::elm::CommKey;
  using CommVolume = vt::elm::CommVolume;
  using CommBytesType = vt::elm::CommBytesType;

  // @todo: should do other types of comm

  {
    PhaseType phase = write_phase;
    lbdh.node_data_[phase];
    lbdh.node_comm_[phase];

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
        lbdh.node_data_[phase][elm_id].whole_phase_load = dur;
        lbdh.node_data_[phase][elm_id].subphase_loads = dur_vec;

        CommKey ntockey(
          CommKey::NodeToCollectionTag{}, this_node, elm_id, false
        );
        CommVolume ntocvol{ntoc, ntocm};
        lbdh.node_comm_[phase][ntockey] = ntocvol;
        lbdh.node_subphase_comm_[phase][i % 2][ntockey] = ntocvol;

        CommKey ctonkey(
          CommKey::CollectionToNodeTag{}, elm_id, this_node, false
        );
        CommVolume ctonvol{cton, ctonm};
        lbdh.node_comm_[phase][ctonkey] = ctonvol;
        lbdh.node_subphase_comm_[phase][(i + 1) % 2][ctonkey] = ctonvol;

        std::vector<uint64_t> arr;
        arr.push_back(idx.x());
        lbdh.node_idx_[elm_id] = std::make_tuple(proxy.getProxy(), arr);
      }
    }
  }

  auto lbdh_read = getLBDataForPhase(write_phase, lbdh);

  // whole-phase loads
  EXPECT_EQ(lbdh_read.node_data_.size(), lbdh.node_data_.size());
  if (lbdh_read.node_data_.size() != lbdh.node_data_.size()) {
    fmt::print(
      "Wrote {} phases of whole-phase load data but read in {} phases",
      lbdh.node_data_.size(), lbdh_read.node_data_.size()
    );
  } else {
    // compare the whole-phase load data in detail
    for (auto &phase_data : lbdh.node_data_) {
      auto phase = phase_data.first;
      EXPECT_FALSE(lbdh_read.node_data_.find(phase) == lbdh_read.node_data_.end());
      if (lbdh_read.node_data_.find(phase) == lbdh_read.node_data_.end()) {
        fmt::print(
          "Phase {} in whole-phase loads were not read in",
          phase
        );
      } else {
        auto &read_load_map = lbdh_read.node_data_[phase];
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
  EXPECT_EQ(lbdh_read.node_idx_.size(), lbdh.node_idx_.size());
  if (lbdh_read.node_idx_.size() != lbdh.node_idx_.size()) {
    fmt::print(
      "Wrote index mapping for {} elements but read in {}",
      lbdh.node_idx_.size(), lbdh_read.node_idx_.size()
    );
  } else {
    // detailed comparison of element id to index mapping
    for (auto &entry : lbdh_read.node_idx_) {
      auto read_elm_id = entry.first;
      EXPECT_FALSE(lbdh.node_idx_.find(read_elm_id) == lbdh.node_idx_.end());
      if (lbdh.node_idx_.find(read_elm_id) == lbdh.node_idx_.end()) {
        fmt::print(
          "Unexpected element ID read in index mapping: "
          "id={}, home={}, curr={}",
          read_elm_id.id, read_elm_id.getHomeNode(), read_elm_id.curr_node
        );
      } else {
        auto orig_idx = lbdh.node_idx_[read_elm_id];
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
  EXPECT_EQ(lbdh_read.node_comm_.size(), lbdh.node_comm_.size());
  if (lbdh_read.node_comm_.size() != lbdh.node_comm_.size()) {
    fmt::print(
      "Wrote {} phases of whole-phase comm data but read in {} phases",
      lbdh.node_comm_.size(), lbdh_read.node_comm_.size()
    );
  }
  // @todo: detailed comparison of whole-phase comm data

  // @todo: compare subphase comm when writing/reading is implemented
  // @todo: detailed comparison of subphase comm data
}

struct TestDumpUserdefinedData : TestParallelHarnessParam<bool> { };

std::string
getJsonStringForPhase(
  vt::PhaseType phase, vt::vrt::collection::balance::LBDataHolder in
) {
  using vt::vrt::collection::balance::LBDataHolder;
  using JSONAppender = vt::util::json::Appender<std::stringstream>;
  std::stringstream ss{std::ios_base::out | std::ios_base::in};
  nlohmann::json metadata;
  metadata["type"] = "LBDatafile";
  auto ap = std::make_unique<JSONAppender>(
    "phases", metadata, std::move(ss), false
  );
  auto j = in.toJson(phase);
  ap->addElm(*j);
  ss = ap->finish();
  return ss.str();
}

TEST_P(TestDumpUserdefinedData, test_dump_userdefined_json) {
  bool should_dump = GetParam();

  auto this_node = vt::theContext()->getNode();
  auto num_nodes = vt::theContext()->getNumNodes();

  vt::vrt::collection::CollectionProxy<MyCol> proxy;
  auto const range = vt::Index1D(num_nodes * 1);

  // Construct a collection
  runInEpochCollective([&] {
    proxy = vt::theCollection()->constructCollective<MyCol>(
      range, "test_dump_userdefined_json"
    );
  });

  vt::vrt::collection::balance::LBDataHolder lbdh;
  PhaseType write_phase = 0;

  {
    PhaseType phase = write_phase;
    lbdh.node_data_[phase];
    lbdh.node_comm_[phase];

    vt::Index1D idx(this_node * 1);
    auto elm_ptr = proxy(idx).tryGetLocalPtr();
    EXPECT_NE(elm_ptr, nullptr);
    if (elm_ptr != nullptr) {
      auto elm_id = elm_ptr->getElmID();
      elm_ptr->valInsert("hello", std::string("world"), should_dump);
      elm_ptr->valInsert("elephant", 123456789, should_dump);
      lbdh.user_defined_json_[phase][elm_id] = std::make_shared<nlohmann::json>(
        elm_ptr->toJson()
      );
      lbdh.node_data_[phase][elm_id].whole_phase_load = 1.0;
    }
  }

  auto json_str = getJsonStringForPhase(write_phase, lbdh);
  fmt::print("{}\n", json_str);
  if (should_dump) {
    EXPECT_NE(json_str.find("user_defined"), std::string::npos);
    EXPECT_NE(json_str.find("hello"), std::string::npos);
    EXPECT_NE(json_str.find("world"), std::string::npos);
    EXPECT_NE(json_str.find("elephant"), std::string::npos);
    EXPECT_NE(json_str.find("123456789"), std::string::npos);
  } else {
    EXPECT_EQ(json_str.find("user_defined"), std::string::npos);
    EXPECT_EQ(json_str.find("hello"), std::string::npos);
    EXPECT_EQ(json_str.find("world"), std::string::npos);
    EXPECT_EQ(json_str.find("elephant"), std::string::npos);
    EXPECT_EQ(json_str.find("123456789"), std::string::npos);
  }
}

auto const booleans = ::testing::Values(false, true);

INSTANTIATE_TEST_SUITE_P(
  DumpUserdefinedDataExplode, TestDumpUserdefinedData, booleans
);

struct SerializationTestCol : vt::Collection<SerializationTestCol, vt::Index1D> {
  template <typename SerializerT> void serialize(SerializerT &s) {
    vt::Collection<SerializationTestCol, vt::Index1D>::serialize(s);

    if (s.isSizing()) {
      s | was_packed | was_unpacked | packed_on_node | unpacked_on_node;
      return;
    }

    was_packed = was_unpacked = false;
    packed_on_node = unpacked_on_node = -1;

    if (s.isPacking()) {
      was_packed = true;
      packed_on_node = theContext()->getNode();
    }

    s | was_packed | was_unpacked | packed_on_node | unpacked_on_node;

    if (s.isUnpacking()) {
      was_unpacked = true;
      unpacked_on_node = theContext()->getNode();
    }
  }

  bool was_packed = false;
  bool was_unpacked = false;
  int packed_on_node = -1;
  int unpacked_on_node = -1;
};

void serializationColHandler(SerializationTestCol *col) {
  auto const cur_phase = thePhase()->getCurrentPhase();
  if (cur_phase < 2) {
    return;
  }

  EXPECT_TRUE(col->was_packed);
  EXPECT_TRUE(col->was_unpacked);
  EXPECT_EQ(col->packed_on_node, col->unpacked_on_node);
}

void runSerializationTest() {
  theConfig()->vt_lb = true;
  theConfig()->vt_lb_self_migration = true;
  theConfig()->vt_lb_name = "TestSerializationLB";
  if (theContext()->getNode() == 0) {
    ::fmt::print("Testing LB: TestSerializationLB\n");
  }

  theCollective()->barrier();

  auto range = Index1D{8};
  vrt::collection::CollectionProxy<SerializationTestCol> proxy;

  runInEpochCollective([&] {
    proxy = theCollection()->constructCollective<SerializationTestCol>(range);
  });

  for (int phase = 0; phase < num_phases; ++phase) {
    runInEpochCollective([&] {
      proxy.broadcastCollective<serializationColHandler>();
    });
    thePhase()->nextPhaseCollective();
  }
}

struct TestLoadBalancerTestSerializationLB : TestParallelHarness {};

TEST_F(TestLoadBalancerTestSerializationLB, test_TestSerializationLB_load_balancer) {
  theCollective()->barrier();
  runSerializationTest();
}

}}}} // end namespace vt::tests::unit::lb

#endif /*vt_check_enabled(lblite)*/
