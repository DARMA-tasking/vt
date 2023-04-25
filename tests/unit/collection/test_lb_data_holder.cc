/*
//@HEADER
// *****************************************************************************
//
//                            test_lb_data_holder.cc
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
#include "test_helpers.h"
#include "test_collection_common.h"

#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/balance/lb_data_holder.h"

#include <nlohmann/json.hpp>

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit { namespace lb {

static constexpr int const num_phases = 10;

struct TestOptionalPhasesMetadata : TestParallelHarnessParam<bool> { };

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
      dh.node_data_[i][elm] = LoadSummary{3};
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

TEST_F(TestOptionalPhasesMetadata, test_no_lb_phases_metadata) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json json;
  json["metadata"]["type"] = "LBDatafile";

  addPhasesDataToJson(json, num_phases, {});

  LBDataHolder testObj(json);
  EXPECT_EQ(testObj.count_, num_phases);
  std::set<PhaseType> expectedSkipped = {};
  EXPECT_EQ(testObj.skipped_phases_, expectedSkipped);
  std::set<PhaseType> expectedIdentical = {};
  EXPECT_EQ(testObj.identical_phases_, expectedIdentical);

  auto outJsonPtr = testObj.metadataToJson();
  ASSERT_TRUE(outJsonPtr != nullptr);

  EXPECT_EQ((*outJsonPtr)["count"], num_phases);
  std::vector<PhaseType> expectedSkippedList = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["list"], expectedSkippedList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedSkippedRanges = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["range"], expectedSkippedRanges);
  std::vector<PhaseType> expectedIdenticalList = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["list"], expectedIdenticalList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedIdenticalRanges = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["range"], expectedIdenticalRanges);
}

TEST_F(TestOptionalPhasesMetadata, test_lb_phases_metadata_empty) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json metadata, phasesMetadata, json;
  phasesMetadata["count"] = num_phases;
  phasesMetadata["skipped"]["list"] = {};
  phasesMetadata["skipped"]["range"] = {};
  phasesMetadata["identical_to_previous"]["list"] = {};
  phasesMetadata["identical_to_previous"]["range"] = {};
  metadata["type"] = "LBDatafile";
  metadata["phases"] = phasesMetadata;
  json["metadata"] = metadata;

  addPhasesDataToJson(json, num_phases, {});

  LBDataHolder testObj(json);
  EXPECT_EQ(testObj.count_, num_phases);
  std::set<PhaseType> expectedSkipped = {};
  EXPECT_EQ(testObj.skipped_phases_, expectedSkipped);
  std::set<PhaseType> expectedIdentical = {};
  EXPECT_EQ(testObj.identical_phases_, expectedIdentical);

  auto outJsonPtr = testObj.metadataToJson();
  ASSERT_TRUE(outJsonPtr != nullptr);

  EXPECT_EQ((*outJsonPtr)["count"], num_phases);
  std::vector<PhaseType> expectedSkippedList = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["list"], expectedSkippedList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedSkippedRanges = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["range"], expectedSkippedRanges);
  std::vector<PhaseType> expectedIdenticalList = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["list"], expectedIdenticalList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedIdenticalRanges = {};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["range"], expectedIdenticalRanges);
}

TEST_F(TestOptionalPhasesMetadata, test_lb_phases_metadata_filled) {
  using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

  nlohmann::json metadata, phasesMetadata, json;
  phasesMetadata["count"] = num_phases;
  phasesMetadata["skipped"]["list"] = {2};
  phasesMetadata["skipped"]["range"] = {{3,4}};
  phasesMetadata["identical_to_previous"]["list"] = {1};
  phasesMetadata["identical_to_previous"]["range"] = {{8,9}};
  metadata["type"] = "LBDatafile";
  metadata["phases"] = phasesMetadata;
  json["metadata"] = metadata;

  addPhasesDataToJson(json, num_phases, {1,2,3,4,8,9});

  LBDataHolder testObj(json);
  EXPECT_EQ(testObj.count_, num_phases);
  std::set<PhaseType> expectedSkipped = {2, 3, 4};
  EXPECT_EQ(testObj.skipped_phases_, expectedSkipped);
  std::set<PhaseType> expectedIdentical = {1, 8, 9};
  EXPECT_EQ(testObj.identical_phases_, expectedIdentical);

  auto outJsonPtr = testObj.metadataToJson();
  ASSERT_TRUE(outJsonPtr != nullptr);

  EXPECT_EQ((*outJsonPtr)["count"], num_phases);
  std::vector<PhaseType> expectedSkippedList = {};
  EXPECT_EQ((*outJsonPtr)["skipped"]["list"], expectedSkippedList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedSkippedRanges = {{2,4}};
  EXPECT_EQ((*outJsonPtr)["skipped"]["range"], expectedSkippedRanges);
  std::vector<PhaseType> expectedIdenticalList = {1};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["list"], expectedIdenticalList);
  std::vector<std::pair<PhaseType, PhaseType>> expectedIdenticalRanges = {{8,9}};
  EXPECT_EQ((*outJsonPtr)["identical_to_previous"]["range"], expectedIdenticalRanges);
}

}}}} // end namespace vt::tests::unit::lb

#endif /*vt_check_enabled(lblite)*/
