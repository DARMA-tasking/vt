/*
//@HEADER
// *****************************************************************************
//
//              test_model_weighted_communication_volume.nompi.cc
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
#include <vt/vrt/collection/balance/model/weighted_communication_volume.h>
#include <vt/vrt/collection/balance/model/weighted_messages.h>

#include <gtest/gtest.h>

#include "test_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace weighted {
namespace volume {

using TestModelWeightedCommunicationVolume = TestHarness;

using vt::elm::CommKeyType;
using vt::elm::CommMapType;
using vt::elm::CommVolume;
using vt::elm::ElementIDStruct;
using vt::vrt::collection::balance::WeightedCommunicationVolume;
using vt::vrt::collection::balance::WeightedMessages;
using vt::vrt::collection::balance::LoadMapType;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::ObjectIterator;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::LoadMapObjectIterator;

using ProcLoadMap = std::unordered_map<PhaseType, LoadMapType>;
using ProcCommMap = std::unordered_map<PhaseType, CommMapType>;

static auto num_phases = 0;

struct StubModel : LoadModel {
  StubModel() = default;
  virtual ~StubModel() = default;

  void setLoads(ProcLoadMap const* proc_load, ProcCommMap const* proc_comm) override {
    proc_load_ = proc_load;
    proc_comm_ = proc_comm;
  }

  void updateLoads(PhaseType) override { }

  TimeType getModeledLoad(ElementIDStruct id, PhaseOffset phase) const override {
    const auto work = proc_load_->at(0).at(id).whole_phase_load;

    if (phase.subphase == PhaseOffset::WHOLE_PHASE) {
      return work;
    } else {
      return work / 10;
    }
  }

  ObjectIterator begin() const override {
    return {std::make_unique<LoadMapObjectIterator>(
      proc_load_->at(0).begin(), proc_load_->at(0).end()
    )};
  }

  unsigned int getNumCompletedPhases() const override { return num_phases; }

  // Not used in this test
  int getNumSubphases() const override { return 0; }
  unsigned int getNumPastPhasesNeeded(unsigned int look_back = 0) const override {
    return look_back;
  }

private:
  ProcLoadMap const* proc_load_ = nullptr;
  ProcCommMap const* proc_comm_ = nullptr;
};

TEST_F(TestModelWeightedCommunicationVolume, test_model) {
  // For simplicity's sake, the elements are on the home node
  // Element 1 (home node == 1)
  ElementIDStruct const elem1 = {1, 1};

  // Element 2 (home node == 2)
  ElementIDStruct const elem2 = {2, 2};

  // Element 3 (home node == 3)
  ElementIDStruct const elem3 = {3, 3};

  ProcLoadMap proc_load = {{0, LoadMapType{{elem2, {TimeType{150}, {}}}}}};

  ProcCommMap proc_comm = {
    {0,
     CommMapType{// Node 1 -> Node 2
                 {{CommKeyType::CollectionTag{}, elem1, elem2, false},
                  CommVolume{20.0, 2}},
                 // Node 3 -> Node 2
                 {{CommKeyType::CollectionTag{}, elem3, elem2, false},
                  CommVolume{5.0, 5}}}
    },
    {1,
     CommMapType{// Node 3 -> Node 2
                 {{CommKeyType::CollectionTag{}, elem3, elem2, false},
                  CommVolume{500.0, 50}},
                 // Node 1 -> Node 2
                 {{CommKeyType::CollectionTag{}, elem1, elem2, false},
                  CommVolume{25.0, 10}}}
    }
  };

  constexpr auto alpha = 0.8;
  constexpr auto beta  = 0.2;
  constexpr auto gamma = 0.0;

  auto weighted_messages_model = std::make_shared<WeightedMessages>(
    std::make_shared<StubModel>()
  );
  auto test_model = std::make_shared<WeightedCommunicationVolume>(
    weighted_messages_model, alpha, beta, gamma
  );
  test_model->setLoads(&proc_load, &proc_comm);

  std::unordered_map<PhaseType, TimeType> expected_work = {
    {0, TimeType{125.0}}, {1, TimeType{22.5}}
  };

  for (; num_phases < 2; ++num_phases) {
    test_model->updateLoads(num_phases);
    int objects_seen = 0;

    for (auto&& obj : *test_model) {
      EXPECT_TRUE(obj.id == 2);
      ++objects_seen;

      const auto subphase = num_phases == 0 ? PhaseOffset::WHOLE_PHASE : 1;
      auto work_val = test_model->getModeledLoad(obj, PhaseOffset{0, subphase});
      EXPECT_EQ(work_val, expected_work[num_phases])
        << fmt::format("For element={} on phase={}\n", obj, num_phases);
    }

    EXPECT_EQ(objects_seen, 1);
  }
}

}}}}} // end namespace vt::tests::unit::weighted::volume
