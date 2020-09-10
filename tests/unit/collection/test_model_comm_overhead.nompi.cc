/*
//@HEADER
// *****************************************************************************
//
//                      test_model_comm_overhead.nompi.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#include <vt/transport.h>
#include <vt/vrt/collection/balance/model/load_model.h>
#include <vt/vrt/collection/balance/model/comm_overhead.h>
#include <vt/vrt/collection/balance/lb_comm.h>

#include <gtest/gtest.h>

#include "test_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit {

using TestModelCommOverhead = TestHarness;

using vt::vrt::collection::balance::CommKeyType;
using vt::vrt::collection::balance::CommMapType;
using vt::vrt::collection::balance::CommOverhead;
using vt::vrt::collection::balance::CommVolume;
using vt::vrt::collection::balance::ElementIDType;
using vt::vrt::collection::balance::LoadMapType;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::ObjectIterator;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::SubphaseLoadMapType;

using ProcLoadMap = std::unordered_map<PhaseType, LoadMapType>;
using ProcSubphaseLoadMap = std::unordered_map<PhaseType, SubphaseLoadMapType>;
using ProcCommMap = std::unordered_map<PhaseType, CommMapType>;

static auto num_phases = 0;

struct StubModel : LoadModel {

  StubModel() = default;
  virtual ~StubModel() = default;

  void setLoads(
    ProcLoadMap const* proc_load, ProcSubphaseLoadMap const*,
    ProcCommMap const*) override {
    proc_load_ = proc_load;
  }

  void updateLoads(PhaseType) override {}

  TimeType getWork(ElementIDType id, PhaseOffset phase) override {
    const auto work = proc_load_->at(0).at(id);

    if (phase.subphase == PhaseOffset::WHOLE_PHASE) {
      return work;
    } else {
      return work / 10;
    }
  }

  ObjectIterator begin() override {
    return ObjectIterator(proc_load_->at(0).begin());
  }
  ObjectIterator end() override {
    return ObjectIterator(proc_load_->at(0).end());
  }

  int getNumCompletedPhases() override { return num_phases; }

  // Not used in this test
  int getNumObjects() override { return 0; }
  int getNumSubphases() override { return 0; }
  int getNumPastPhasesNeeded(int look_back = 0) override { return look_back; }

private:
  ProcLoadMap const* proc_load_ = nullptr;
};

TEST_F(TestModelCommOverhead, test_model_comm_overhead_1) {
  ProcLoadMap proc_load = {
    {0,
     LoadMapType{
       {ElementIDType{1}, TimeType{60}},
       {ElementIDType{2}, TimeType{150}},
       {ElementIDType{3}, TimeType{75}}}}};

  ProcCommMap proc_comm = {
    {0,
     CommMapType{// Node 1 -> Node 2
                 {{CommKeyType::CollectionTag{}, ElementIDType{1},
                   ElementIDType{1}, ElementIDType{2}, ElementIDType{2}, false},
                  CommVolume{20.0, 2}},

                 // Node 3 -> Node 2
                 {{CommKeyType::CollectionTag{}, ElementIDType{3},
                   ElementIDType{3}, ElementIDType{2}, ElementIDType{2}, false},
                  CommVolume{5.0, 5}},

                 // Node 1 -> Node 3
                 {{CommKeyType::CollectionTag{}, ElementIDType{1},
                   ElementIDType{1}, ElementIDType{3}, ElementIDType{3}, false},
                  CommVolume{100.0, 1}}}},
    {1,
     CommMapType{// Node 3 -> Node 1 (current phase) -> Node 2
                 {{CommKeyType::CollectionTag{}, ElementIDType{3},
                   ElementIDType{3}, ElementIDType{2}, ElementIDType{1}, false},
                  CommVolume{20.0, 2}},

                 // Node 3 -> Node 2 (current phase) -> Node 1
                 {{CommKeyType::CollectionTag{}, ElementIDType{3},
                   ElementIDType{3}, ElementIDType{1}, ElementIDType{2}, false},
                  CommVolume{500.0, 50}},

                 // Node 1 -> Node 3 (current phase) -> Node 2
                 {{CommKeyType::CollectionTag{}, ElementIDType{1},
                   ElementIDType{1}, ElementIDType{2}, ElementIDType{3}, false},
                  CommVolume{25.0, 10}}}}};

  auto test_model =
    std::make_shared<CommOverhead>(std::make_shared<StubModel>(), 3.0, 5.0);
  test_model->setLoads(&proc_load, nullptr, &proc_comm);

  std::unordered_map<PhaseType, std::vector<TimeType>> expected_work = {
    {0, {TimeType{60}, TimeType{296}, TimeType{578}}},
    {1, {TimeType{16.6}, TimeType{280}, TimeType{23}}}};

  for (; num_phases < 2; ++num_phases) {
    test_model->updateLoads(num_phases);
    int objects_seen = 0;

    for (auto&& obj : *test_model) {
      EXPECT_TRUE(obj == 1 || obj == 2 || obj == 3);
      ++objects_seen;

      const auto subphase = num_phases == 0 ? PhaseOffset::WHOLE_PHASE : 1;
      auto work_val = test_model->getWork(obj, PhaseOffset{0, subphase});
      EXPECT_EQ(work_val, expected_work[num_phases][obj - 1])
        << fmt::format("For element={} on phase={}\n", obj, num_phases);
    }

    EXPECT_EQ(objects_seen, 3);
  }
}

}}} // end namespace vt::tests::unit
