/*
//@HEADER
// *****************************************************************************
//
//                           test_model_norm.nompi.cc
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

#include <vt/vrt/collection/balance/model/load_model.h>
#include <vt/vrt/collection/balance/model/norm.h>

#include <gtest/gtest.h>

#include "test_harness.h"

#include <limits>
#include <memory>

namespace vt { namespace tests { namespace unit { namespace norm {

using TestModelNorm = TestHarness;

using vt::vrt::collection::balance::CommMapType;
using vt::vrt::collection::balance::ElementIDStruct;
using vt::vrt::collection::balance::LoadMapType;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::Norm;
using vt::vrt::collection::balance::ObjectIterator;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::SubphaseLoadMapType;
using vt::vrt::collection::balance::LoadMapObjectIterator;
using vt::vrt::collection::balance::DataMapType;

using ProcLoadMap = vt::util::container::CircularPhasesBuffer<LoadMapType>;
using ProcSubphaseLoadMap = vt::util::container::CircularPhasesBuffer<SubphaseLoadMapType>;
using ProcCommMap = vt::util::container::CircularPhasesBuffer<CommMapType>;
using UserDataMap = vt::util::container::CircularPhasesBuffer<DataMapType>;

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
    return {std::make_unique<LoadMapObjectIterator>(proc_load_->at(0).begin(), proc_load_->at(0).end())};
  }

  int getNumSubphases() const override { return num_subphases; }

  // Not used in this test
  unsigned int getNumCompletedPhases() const override { return 0; }
  unsigned int getNumPastPhasesNeeded(unsigned int look_back = 0) const override { return look_back; }

private:
  ProcLoadMap const* proc_load_ = nullptr;
};

TEST_F(TestModelNorm, test_model_norm_1) {
  NodeType this_node = 0;
  ProcLoadMap proc_load = {
    {0,
     LoadMapType{
       {ElementIDStruct{1,this_node}, {LoadType{60}, {LoadType{10}, LoadType{20}, LoadType{30}}}},
       {ElementIDStruct{2,this_node}, {LoadType{150}, {LoadType{40}, LoadType{50}, LoadType{60}}}}}}};

  auto test_model = std::make_shared<Norm>(std::make_shared<StubModel>(), 3.0);
  test_model->setLoads(&proc_load, nullptr, nullptr);
  test_model->updateLoads(0);

  // ONLY because this is built on top of the StubModel do we expect false
  EXPECT_FALSE(test_model->hasRawLoad());

  for (unsigned int iter = 0; iter < num_subphases; ++iter) {
    int objects_seen = 0;
    for (auto&& obj : *test_model) {
      EXPECT_TRUE(obj.id == 1 || obj.id == 2);
      ++objects_seen;

      // offset.subphase != PhaseOffset::WHOLE_PHASE
      // expect work load value for given subphase
      auto work_val = test_model->getModeledLoad(obj, PhaseOffset{0, iter});
      EXPECT_EQ(work_val, proc_load[0][obj].subphase_loads[iter]);
    }

    EXPECT_EQ(objects_seen, 2);
  }
}

TEST_F(TestModelNorm, test_model_norm_2) {
  NodeType this_node = 0;
  ProcLoadMap proc_load = {
    {0,
     LoadMapType{
       {ElementIDStruct{1,this_node}, {LoadType{60}, {LoadType{10}, LoadType{20}, LoadType{30}}}},
       {ElementIDStruct{2,this_node}, {LoadType{150}, {LoadType{40}, LoadType{50}, LoadType{60}}}}}}};

  // finite 'power' value
  auto test_model = std::make_shared<Norm>(std::make_shared<StubModel>(), 3.0);
  test_model->setLoads(&proc_load, nullptr, nullptr);
  test_model->updateLoads(0);

  std::array<LoadType, 2> expected_norms = {
    LoadType{33.019}, LoadType{73.986}};

  int objects_seen = 0;
  for (auto&& obj : *test_model) {
    EXPECT_TRUE(obj.id == 1 || obj.id == 2);
    ++objects_seen;

    auto work_val =
      test_model->getModeledLoad(obj, PhaseOffset{0, PhaseOffset::WHOLE_PHASE});
    EXPECT_NEAR(work_val, expected_norms[obj.id - 1], 0.001);
  }

  EXPECT_EQ(objects_seen, 2);
}

TEST_F(TestModelNorm, test_model_norm_3) {
  NodeType this_node = 0;
  ProcLoadMap proc_load = {
    {0,
     LoadMapType{
       {ElementIDStruct{1,this_node}, {LoadType{60}, {LoadType{10}, LoadType{20}, LoadType{30}}}},
       {ElementIDStruct{2,this_node}, {LoadType{150}, {LoadType{40}, LoadType{50}, LoadType{60}}}}}}};

  // infinite 'power' value
  auto test_model = std::make_shared<Norm>(
    std::make_shared<StubModel>(), std::numeric_limits<double>::infinity());
  test_model->setLoads(&proc_load, nullptr, nullptr);
  test_model->updateLoads(0);

  std::array<LoadType, 2> expected_norms = {LoadType{30}, LoadType{60}};

  int objects_seen = 0;
  for (auto&& obj : *test_model) {
    EXPECT_TRUE(obj.id == 1 || obj.id == 2);
    ++objects_seen;

    auto work_val =
      test_model->getModeledLoad(obj, PhaseOffset{0, PhaseOffset::WHOLE_PHASE});
    EXPECT_EQ(work_val, expected_norms[obj.id - 1]);
  }

  EXPECT_EQ(objects_seen, 2);
}

}}}} // end namespace vt::tests::unit::norm
