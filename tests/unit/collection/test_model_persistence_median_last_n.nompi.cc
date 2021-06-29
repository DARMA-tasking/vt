/*
//@HEADER
// *****************************************************************************
//
//                test_model_persistence_median_last_n.nompi.cc
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

#include <vt/vrt/collection/balance/model/load_model.h>
#include <vt/vrt/collection/balance/model/persistence_median_last_n.h>

#include <gtest/gtest.h>

#include "test_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace persistence {

using TestModelPersistenceMedianLastN = TestHarness;

static int32_t num_phases = 0;

using vt::vrt::collection::balance::ElementIDStruct;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::PersistenceMedianLastN;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::LoadMapType;
using vt::vrt::collection::balance::SubphaseLoadMapType;
using vt::vrt::collection::balance::CommMapType;
using vt::vrt::collection::balance::ObjectIterator;

struct StubModel : LoadModel {

  StubModel() = default;
  virtual ~StubModel() = default;

  void setLoads(
    std::unordered_map<PhaseType, LoadMapType> const* proc_load,
    std::unordered_map<PhaseType, SubphaseLoadMapType> const*,
    std::unordered_map<PhaseType, CommMapType> const*) override {
    proc_load_ = proc_load;
  }

  void updateLoads(PhaseType) override {}

  TimeType getWork(ElementIDStruct id, PhaseOffset phase) override {
    // Most recent phase will be at the end of vector
    return proc_load_->at(num_phases + phase.phases).at(id);
  }

  virtual ObjectIterator begin() override {
    return ObjectIterator(proc_load_->at(num_phases-1).begin());
  }
  virtual ObjectIterator end() override {
    return ObjectIterator(proc_load_->at(num_phases-1).end());
  }

  virtual int getNumObjects() override { return 2; }
  virtual unsigned int getNumCompletedPhases() override { return num_phases; }
  virtual int getNumSubphases() override { return 1; }
  unsigned int getNumPastPhasesNeeded(unsigned int look_back = 0) override { return look_back; }

private:
  std::unordered_map<PhaseType, LoadMapType> const* proc_load_ = nullptr;
};

TEST_F(TestModelPersistenceMedianLastN, test_model_persistence_median_last_n_1) {
  constexpr int32_t num_total_phases = 7;
  NodeType this_node = 0;

  auto test_model =
    std::make_shared<PersistenceMedianLastN>(std::make_shared<StubModel>(), 4);

  std::unordered_map<PhaseType, LoadMapType> proc_loads(num_total_phases);

  test_model->setLoads(&proc_loads, nullptr, nullptr);

  // Work loads to be added in each test iteration
  std::vector<LoadMapType> load_holder{
    LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{10}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{40}}},
    LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{4}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{10}}},
    LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{20}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{100}}},
    LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{50}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{40}}},
    LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{2}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{50}}},
    LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{60}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{20}}},
    LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{100}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{10}}},
  };

  std::array<std::pair<TimeType, TimeType>, num_total_phases> expected_medians{
    std::make_pair(TimeType{10}, TimeType{40}), // iter 0 results
    std::make_pair(TimeType{7},  TimeType{25}), // iter 1 results
    std::make_pair(TimeType{10}, TimeType{40}), // iter 2 results
    std::make_pair(TimeType{15}, TimeType{40}), // iter 3 results
    std::make_pair(TimeType{12}, TimeType{45}), // iter 4 results
    std::make_pair(TimeType{35}, TimeType{45}), // iter 5 results
    std::make_pair(TimeType{55}, TimeType{30})  // iter 6 results
  };

  for (auto iter = 0; iter < num_total_phases; ++iter) {
    proc_loads[iter] = load_holder[iter];
    test_model->updateLoads(iter);
    ++num_phases;

    for (auto&& obj : *test_model) {
      auto work_val = test_model->getWork(obj, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE});
      EXPECT_EQ(
        work_val,
        obj.id == 1 ? expected_medians[iter].first : expected_medians[iter].second)
        << fmt::format("Test failed on iteration {}", iter);
    }
  }
}

}}}} // end namespace vt::tests::unit::persistence
