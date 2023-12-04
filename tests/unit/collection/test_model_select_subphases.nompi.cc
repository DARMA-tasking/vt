/*
//@HEADER
// *****************************************************************************
//
//                     test_model_select_subphases.nompi.cc
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
#include <vt/vrt/collection/balance/model/select_subphases.h>

#include <gtest/gtest.h>

#include "test_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace select {

using TestModelSelectSubphases = TestHarness;

using vt::vrt::collection::balance::CommMapType;
using vt::vrt::collection::balance::ElementIDStruct;
using vt::vrt::collection::balance::LoadMapType;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::ObjectIterator;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::SelectSubphases;
using vt::vrt::collection::balance::SubphaseLoadMapType;
using vt::vrt::collection::balance::LoadMapObjectIterator;
using vt::vrt::collection::balance::DataMapType;

using ProcLoadMap = std::unordered_map<PhaseType, LoadMapType>;
using ProcSubphaseLoadMap = std::unordered_map<PhaseType, SubphaseLoadMapType>;
using ProcCommMap = std::unordered_map<PhaseType, CommMapType>;
using UserDataMap = std::unordered_map<PhaseType, DataMapType>;

constexpr auto num_subphases = 3;

struct StubModel : LoadModel {

  StubModel() = default;
  virtual ~StubModel() = default;

  void setLoads(
    ProcLoadMap const* proc_load,
    ProcCommMap const*,
    UserDataMap const*) override {
    proc_load_ = proc_load;
  }

  void updateLoads(PhaseType) override {}

  LoadType getModeledLoad(ElementIDStruct id, PhaseOffset phase) const override {
    return proc_load_->at(0).at(id).subphase_loads.at(phase.subphase);
  }

  ObjectIterator begin() const override {
    return {
      std::make_unique<LoadMapObjectIterator>(
        proc_load_->at(0).begin(), proc_load_->at(0).end()
      )
    };
  }

  int getNumSubphases() const override { return num_subphases; }

  // Not used in this test
  unsigned int getNumCompletedPhases() const override { return 0; }
  unsigned int getNumPastPhasesNeeded(unsigned int look_back = 0) const override {
    return look_back;
  }

private:
  ProcLoadMap const* proc_load_ = nullptr;
};

TEST_F(TestModelSelectSubphases, test_model_select_subphases_1) {
  NodeType this_node = 0;
  ElementIDStruct id1{1,this_node};
  ElementIDStruct id2{2,this_node};

  ProcLoadMap proc_load = {
    {0,
     LoadMapType{
       {id1, {LoadType{60}, {LoadType{10}, LoadType{20}, LoadType{30}}}},
       {id2, {LoadType{150}, {LoadType{40}, LoadType{50}, LoadType{60}}}}}}};

  std::vector<unsigned int> subphases{2, 0, 1};
  auto test_model =
    std::make_shared<SelectSubphases>(std::make_shared<StubModel>(), subphases);

  EXPECT_EQ(test_model->getNumSubphases(), subphases.size());

  test_model->setLoads(&proc_load, nullptr, nullptr);
  test_model->updateLoads(0);

  std::unordered_map<ElementIDStruct, std::vector<LoadType>> expected_values = {
    {id1,
     {proc_load[0][id1].subphase_loads[subphases[0]],
      proc_load[0][id1].subphase_loads[subphases[1]],
      proc_load[0][id1].subphase_loads[subphases[2]]}},
    {id2,
     {proc_load[0][id2].subphase_loads[subphases[0]],
      proc_load[0][id2].subphase_loads[subphases[1]],
      proc_load[0][id2].subphase_loads[subphases[2]]}}};

  for (unsigned int iter = 0; iter < num_subphases; ++iter) {
    int objects_seen = 0;

    for (auto&& obj : *test_model) {
      EXPECT_TRUE(obj.id == 1 || obj.id == 2);
      ++objects_seen;

      // offset.subphase != PhaseOffset::WHOLE_PHASE
      // expect work load value for given subphase
      auto work_val = test_model->getModeledLoad(obj, PhaseOffset{0, iter});
      EXPECT_EQ(work_val, expected_values[obj][iter]);
    }

    EXPECT_EQ(objects_seen, 2);
  }
}

TEST_F(TestModelSelectSubphases, test_model_select_subphases_2) {
  NodeType this_node = 0;
  ProcLoadMap proc_load = {
    {0,
     LoadMapType{
       {ElementIDStruct{1,this_node},
        {LoadType{60}, {LoadType{10}, LoadType{20}, LoadType{30}}}},
       {ElementIDStruct{2,this_node},
        {LoadType{150}, {LoadType{40}, LoadType{50}, LoadType{60}}}}
     }
    }
  };

  std::vector<unsigned int> subphases{2, 1};
  auto test_model =
    std::make_shared<SelectSubphases>(std::make_shared<StubModel>(), subphases);

  EXPECT_EQ(test_model->getNumSubphases(), subphases.size());

  test_model->setLoads(&proc_load, nullptr, nullptr);
  test_model->updateLoads(0);

  std::unordered_map<ElementIDStruct, LoadType> expected_values = {
    {ElementIDStruct{1,this_node}, LoadType{50}},
    {ElementIDStruct{2,this_node}, LoadType{110}}};

  int objects_seen = 0;

  for (auto&& obj : *test_model) {
    EXPECT_TRUE(obj.id == 1 || obj.id == 2);
    ++objects_seen;

    auto work_val =
      test_model->getModeledLoad(obj, PhaseOffset{0, PhaseOffset::WHOLE_PHASE});
    EXPECT_EQ(work_val, expected_values[obj]);
  }

  EXPECT_EQ(objects_seen, 2);
}

}}}} // end namespace vt::tests::unit::select
