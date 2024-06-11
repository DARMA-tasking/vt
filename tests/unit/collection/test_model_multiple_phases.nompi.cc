/*
//@HEADER
// *****************************************************************************
//
//                     test_model_multiple_phases.nompi.cc
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
#include <vt/vrt/collection/balance/model/multiple_phases.h>

#include <gtest/gtest.h>

#include "test_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace multiple {

using TestModelMultiplePhases = TestHarness;

using vt::vrt::collection::balance::ElementIDStruct;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::MultiplePhases;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::LoadMapType;
using vt::vrt::collection::balance::SubphaseLoadMapType;
using vt::vrt::collection::balance::CommMapType;
using vt::vrt::collection::balance::ObjectIterator;
using vt::vrt::collection::balance::LoadMapObjectIterator;
using vt::vrt::collection::balance::DataMapType;
using vt::vrt::collection::balance::LoadMapBufferType;
using vt::vrt::collection::balance::CommMapBufferType;
using vt::vrt::collection::balance::DataMapBufferType;

struct StubModel : LoadModel {

  StubModel() = default;
  virtual ~StubModel() = default;

  void setLoads(
    LoadMapBufferType const* proc_load,
    CommMapBufferType const*,
    DataMapBufferType const*) override {
    proc_load_ = proc_load;
  }

  void updateLoads(PhaseType) override {}

  LoadType getModeledLoad(ElementIDStruct id, PhaseOffset phase) const override {
    // Here we return predicted loads for future phases
    // For the sake of the test we use values from the past phases
    return proc_load_->at(phase.phases).at(id).whole_phase_load;
  }

  virtual ObjectIterator begin() const override {
    return {std::make_unique<LoadMapObjectIterator>(proc_load_->at(3).begin(), proc_load_->at(3).end())};
  }

  // Not used by this test
  virtual unsigned int getNumCompletedPhases() const override { return 0; }
  virtual int getNumSubphases() const override { return 0; }
  unsigned int getNumPastPhasesNeeded(unsigned int look_back = 0) const override { return look_back; }

private:
  LoadMapBufferType const* proc_load_ = nullptr;
};

TEST_F(TestModelMultiplePhases, test_model_multiple_phases_1) {
  NodeType this_node = 0;
  LoadMapBufferType proc_loads = {
    {0, LoadMapType{
      {ElementIDStruct{1,this_node}, {LoadType{10}, {}}},
      {ElementIDStruct{2,this_node}, {LoadType{40}, {}}}}},
    {1, LoadMapType{
      {ElementIDStruct{1,this_node}, {LoadType{20}, {}}},
      {ElementIDStruct{2,this_node}, {LoadType{30}, {}}}}},
    {2, LoadMapType{
      {ElementIDStruct{1,this_node}, {LoadType{30}, {}}},
      {ElementIDStruct{2,this_node}, {LoadType{10}, {}}}}},
    {3, LoadMapType{
      {ElementIDStruct{1,this_node}, {LoadType{40}, {}}},
      {ElementIDStruct{2,this_node}, {LoadType{5}, {}}}}}};

  auto test_model =
    std::make_shared<MultiplePhases>(std::make_shared<StubModel>(), 4);

  test_model->setLoads(&proc_loads, nullptr, nullptr);
  test_model->updateLoads(3);

  for (auto&& obj : *test_model) {
    auto work_val = test_model->getModeledLoad(
      obj, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
    );
    EXPECT_EQ(work_val, obj.id == 1 ? LoadType{100} : LoadType{85});
  }
}

}}}} // end namespace vt::tests::unit::multiple
