/*
//@HEADER
// *****************************************************************************
//
//                            test_lb_data_holder.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "test_helpers.h"
#include "test_collection_common.h"

#include "vt/elm/elm_id_bits.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/balance/lb_data_holder.h"

#include <nlohmann/json.hpp>

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit { namespace lb {

static constexpr int const num_phases = 10;

struct TestLBDataHolder : TestParallelHarnessParam<bool> { };

void addPhasesDataToJson(nlohmann::json& json, PhaseType amountOfPhasesToAdd, std::vector<PhaseType> toSkip) {
  using ElementIDStruct = vt::vrt::collection::balance::ElementIDStruct;
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;
  using LoadSummary = vt::vrt::collection::balance::LoadSummary;

  std::unordered_map<PhaseType, std::vector<ElementIDStruct>> ids;
  for (unsigned i = 0; i < 2; i++) {
    auto id = elm::ElmIDBits::createCollectionImpl(true, i+1, 0, 0);
    for(unsigned j = 0; j < amountOfPhasesToAdd; j++) {
      ids[j].push_back(id);
    }
  }

  LBDataHolder dh;
  for (unsigned i = 0; i < amountOfPhasesToAdd; i++) {
    for (auto&& elm : ids[i]) {
      dh.node_data_[i][elm] = LoadSummary{3.};
    }
  }

  nlohmann::json phases = nlohmann::json::array();
  for (unsigned i = 0; i < num_phases; i++) {
    if(std::find(toSkip.begin(), toSkip.end(), i) == toSkip.end()) {
      phases.push_back(*dh.toJson(i));
    }
  }

  json["phases"] = phases;
}

nlohmann::json createEntity_(std::string id_type, int id, int home, bool is_migratable) {
  nlohmann::json entity = {
    {id_type, id},
    {"type", "object"},
    {"collection_id", 7},
    {"index", {0}},
    {"home", home},
    {"migratable", is_migratable},
  };
  return entity;
}

nlohmann::json createJson_(
  std::string id_type, int id_1, int id_2, int home, int node,
  bool is_migratable) {

  auto entity_1 = createEntity_(id_type, id_1, home, is_migratable);
  auto entity_2 = createEntity_(id_type, id_2, home, is_migratable);

  // Generate JSON
  nlohmann::json j = {
    {"metadata", {{"rank", 0}, {"type", "LBDatafile"}}},
    {"phases",
      {{{"communications",
        {{{"bytes", 2.0},
          {"from", entity_1},
          {"messages", 1},
          {"to", entity_2},
          {"type", "SendRecv"}}}},
        {"id", 0},
        {"tasks",
        {
          {
            {"entity", entity_1},
            {"node", node},
            {"resource", "cpu"},
            {"time", 0.5},
          },
          {
            {"entity", entity_2},
            {"node", node},
            {"resource", "cpu"},
            {"time", 0.5},
          }
        }
      }}}
    }
  };

  return j;
}

void testDataHolderElms(int seq_id_1, int home, int node, bool is_migratable) {
  // Create a second seq_id
  auto seq_id_2 = seq_id_1 + 1;

  // Determine encoded ID
  auto elm_1 =
    elm::ElmIDBits::createCollectionImpl(is_migratable, seq_id_1, home, node);
  auto encoded_id_1 = elm_1.id;

  // Create second encoded ID
  auto elm_2 =
    elm::ElmIDBits::createCollectionImpl(is_migratable, seq_id_2, home, node);
  auto encoded_id_2 = elm_2.id;

  // Create DataHolder and get resulting object elm
  auto simple_json_id = createJson_(
    "id", encoded_id_1, encoded_id_2, home, node, is_migratable);
  auto dh_id = vt::vrt::collection::balance::LBDataHolder(simple_json_id);

  // Create new DataHolder using "seq_id" and get elm
  auto simple_json_seq =
    createJson_("seq_id", seq_id_1, seq_id_2, home, node, is_migratable);
  auto dh_seq = vt::vrt::collection::balance::LBDataHolder(simple_json_seq);

  // Assert that both elms exist in both DataHolders
  ASSERT_NE(dh_id.node_data_[0].find(elm_1), dh_id.node_data_[0].end());
  ASSERT_NE(dh_seq.node_data_[0].find(elm_1), dh_seq.node_data_[0].end());
  ASSERT_NE(dh_id.node_data_[0].find(elm_2), dh_id.node_data_[0].end());
  ASSERT_NE(dh_seq.node_data_[0].find(elm_2), dh_seq.node_data_[0].end());

  // Check the communication data
  auto comm_id = dh_id.node_comm_[0];
  auto comm_key_id = comm_id.begin()->first;
  auto comm_seq = dh_seq.node_comm_[0];
  auto comm_key_seq = comm_seq.begin()->first;

  // Ensure that we get the same CommKey from both id types
  EXPECT_EQ(comm_key_id, comm_key_seq);

  // Assert that both elms are present in the communication data
  EXPECT_EQ(comm_key_id.fromObj(), elm_1);
  EXPECT_EQ(comm_key_id.toObj(), elm_2);
  EXPECT_EQ(comm_key_seq.fromObj(), elm_1);
  EXPECT_EQ(comm_key_seq.toObj(), elm_2);
}

TEST_F(TestLBDataHolder, test_no_metadata) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json json;
  addPhasesDataToJson(json, num_phases, {});

  LBDataHolder testObj(json);
  std::set<PhaseType> expectedSkipped = {};
  EXPECT_EQ(testObj.skipped_phases_, expectedSkipped);
  std::set<PhaseType> expectedIdentical = {};
  EXPECT_EQ(testObj.identical_phases_, expectedIdentical);

  auto outJsonPtr = testObj.metadataToJson();
  ASSERT_TRUE(outJsonPtr != nullptr);

  std::vector<PhaseType> expectedSkippedList = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["list"], expectedSkippedList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedSkippedRanges = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["range"], expectedSkippedRanges);
  std::vector<PhaseType> expectedIdenticalList = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["list"], expectedIdenticalList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedIdenticalRanges = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["range"], expectedIdenticalRanges);
}

TEST_F(TestLBDataHolder, test_no_lb_phases_metadata) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json json;
  json["metadata"]["type"] = "LBDatafile";

  addPhasesDataToJson(json, num_phases, {});

  LBDataHolder testObj(json);
  std::set<PhaseType> expectedSkipped = {};
  EXPECT_EQ(testObj.skipped_phases_, expectedSkipped);
  std::set<PhaseType> expectedIdentical = {};
  EXPECT_EQ(testObj.identical_phases_, expectedIdentical);

  auto outJsonPtr = testObj.metadataToJson();
  ASSERT_TRUE(outJsonPtr != nullptr);

  std::vector<PhaseType> expectedSkippedList = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["list"], expectedSkippedList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedSkippedRanges = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["range"], expectedSkippedRanges);
  std::vector<PhaseType> expectedIdenticalList = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["list"], expectedIdenticalList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedIdenticalRanges = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["range"], expectedIdenticalRanges);
}

TEST_F(TestLBDataHolder, test_lb_phases_metadata_empty) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json metadata, phasesMetadata, json;
  phasesMetadata["skipped"]["list"] = {};
  phasesMetadata["skipped"]["range"] = {};
  phasesMetadata["identical_to_previous"]["list"] = {};
  phasesMetadata["identical_to_previous"]["range"] = {};
  metadata["type"] = "LBDatafile";
  metadata["phases"] = phasesMetadata;
  json["metadata"] = metadata;

  addPhasesDataToJson(json, num_phases, {});

  LBDataHolder testObj(json);
  std::set<PhaseType> expectedSkipped = {};
  EXPECT_EQ(testObj.skipped_phases_, expectedSkipped);
  std::set<PhaseType> expectedIdentical = {};
  EXPECT_EQ(testObj.identical_phases_, expectedIdentical);

  auto outJsonPtr = testObj.metadataToJson();
  ASSERT_TRUE(outJsonPtr != nullptr);

  std::vector<PhaseType> expectedSkippedList = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["list"], expectedSkippedList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedSkippedRanges = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["range"], expectedSkippedRanges);
  std::vector<PhaseType> expectedIdenticalList = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["list"], expectedIdenticalList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedIdenticalRanges = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["range"], expectedIdenticalRanges);
}

TEST_F(TestLBDataHolder, test_lb_phases_metadata_filled) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json metadata, phasesMetadata, json;
  phasesMetadata["skipped"]["list"] = {2};
  phasesMetadata["skipped"]["range"] = {{3,4}};
  phasesMetadata["identical_to_previous"]["list"] = {1};
  phasesMetadata["identical_to_previous"]["range"] = {{8,9}};
  metadata["type"] = "LBDatafile";
  metadata["phases"] = phasesMetadata;
  json["metadata"] = metadata;

  addPhasesDataToJson(json, num_phases, {1,2,3,4,8,9});

  LBDataHolder testObj(json);
  std::set<PhaseType> expectedSkipped = {2, 3, 4};
  EXPECT_EQ(testObj.skipped_phases_, expectedSkipped);
  std::set<PhaseType> expectedIdentical = {1, 8, 9};
  EXPECT_EQ(testObj.identical_phases_, expectedIdentical);

  auto outJsonPtr = testObj.metadataToJson();
  ASSERT_TRUE(outJsonPtr != nullptr);

  std::vector<PhaseType> expectedSkippedList = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["list"], expectedSkippedList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedSkippedRanges = {{2,4}};
  EXPECT_EQ((*outJsonPtr)["skipped"]["range"], expectedSkippedRanges);
  std::vector<PhaseType> expectedIdenticalList = {1};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["list"], expectedIdenticalList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedIdenticalRanges = {{8,9}};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["range"], expectedIdenticalRanges);
}

TEST_F(TestLBDataHolder, test_lb_rank_attributes) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json json = R"(
    {
      "type": "LBDatafile",
      "metadata" : {
        "attributes": {
            "intSample": 123,
            "doubleSample": 1.99,
            "stringSample": "abc"
        }
      },
      "phases" : []
    }
  )"_json;

  LBDataHolder testObj(json);
  EXPECT_EQ(123, std::get<int>(testObj.rank_attributes_["intSample"]));
  EXPECT_EQ(1.99, std::get<double>(testObj.rank_attributes_["doubleSample"]));
  EXPECT_EQ("abc", std::get<std::string>(testObj.rank_attributes_["stringSample"]));

  auto outAttributesJsonPtr = testObj.rankAttributesToJson();
  ASSERT_TRUE(outAttributesJsonPtr != nullptr);
  EXPECT_EQ(123, (*outAttributesJsonPtr)["attributes"]["intSample"]);
  EXPECT_EQ(1.99, (*outAttributesJsonPtr)["attributes"]["doubleSample"]);
  EXPECT_EQ("abc", (*outAttributesJsonPtr)["attributes"]["stringSample"]);
}

TEST_F(TestLBDataHolder, test_lb_entity_attributes) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json json = R"(
    {
      "type": "LBDatafile",
      "phases": [
        {
          "id": 0,
          "tasks": [
            {
              "entity": {
                "home": 0,
                "id": 524291,
                "type": "object",
                "migratable": true
              },
              "node": 0,
              "resource": "cpu",
              "time": 3.0,
              "attributes": {
                "intSample": 123,
                "doubleSample": 1.99,
                "stringSample": "abc"
              }
            }
          ]
        }
      ]
    }
  )"_json;
  auto id = vt::vrt::collection::balance::ElementIDStruct{524291, 0};

  LBDataHolder testObj(json);
  EXPECT_TRUE(testObj.node_user_attributes_.contains(0));
  EXPECT_TRUE(testObj.node_user_attributes_[0].find(id) != testObj.node_user_attributes_[0].end());
  auto attributes = testObj.node_user_attributes_[0][id];
  EXPECT_EQ(123, std::get<int>(attributes["intSample"]));
  EXPECT_EQ(1.99, std::get<double>(attributes["doubleSample"]));
  EXPECT_EQ("abc", std::get<std::string>(attributes["stringSample"]));

  auto outJsonPtr = testObj.toJson(0);
  ASSERT_TRUE(outJsonPtr != nullptr);
  EXPECT_EQ(123, (*outJsonPtr)["tasks"][0]["attributes"]["intSample"]);
  EXPECT_EQ(1.99, (*outJsonPtr)["tasks"][0]["attributes"]["doubleSample"]);
  EXPECT_EQ("abc", (*outJsonPtr)["tasks"][0]["attributes"]["stringSample"]);
}

TEST_F(TestLBDataHolder, test_default_time_format) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json json;
  json["metadata"]["type"] = "LBDatafile";

  addPhasesDataToJson(json, num_phases, {});

  LBDataHolder testObj(json);
  auto outJsonPtr = testObj.toJson(0);
  ASSERT_TRUE(outJsonPtr != nullptr);

  for (auto& task: (*outJsonPtr)["tasks"]) {
    EXPECT_EQ(true, task["time"].is_number_float());
  }
}

TEST_F(TestLBDataHolder, test_lb_data_holder_object_id) {
  // Run a variety of test cases (seq_id, home, node, is_migratable)
  auto current_node = theContext()->getNode();
  testDataHolderElms(0, 0, current_node, false);
  testDataHolderElms(0, 0, current_node, true);
  testDataHolderElms(1, 0, current_node, false);
  testDataHolderElms(1, 0, current_node, true);
  testDataHolderElms(0, 1, current_node, false);
  testDataHolderElms(0, 1, current_node, true);
  testDataHolderElms(3, 1, current_node, false);
  testDataHolderElms(2, 2, current_node, true);
}

}}}} // end namespace vt::tests::unit::lb

#endif /*vt_check_enabled(lblite)*/
