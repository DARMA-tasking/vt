/*
//@HEADER
// *****************************************************************************
//
//                         test_model_raw_data.nompi.cc
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
#include <vt/vrt/collection/balance/model/raw_data.h>

#include <gtest/gtest.h>

#include "test_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace raw {

using TestRawData = TestHarness;

using vt::vrt::collection::balance::CommMapType;
using vt::vrt::collection::balance::ElementIDStruct;
using vt::vrt::collection::balance::RawData;
using vt::vrt::collection::balance::LoadMapType;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::ObjectIterator;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::SubphaseLoadMapType;

TEST_F(TestRawData, test_model_raw_data_scalar) {
  NodeType this_node = 0;
  auto test_model =
    std::make_shared<RawData>();

  std::unordered_map<PhaseType, LoadMapType> proc_loads;
  test_model->setLoads(&proc_loads, nullptr);
  EXPECT_TRUE(test_model->hasRawLoad());

  ElementIDStruct id1{1,this_node};
  ElementIDStruct id2{2,this_node};

  // Work loads to be added in each test iteration
  std::vector<LoadMapType> load_holder{
    LoadMapType{{id1, {TimeType{5}, {TimeType{5}}}},   {id2, {TimeType{10}, {TimeType{10}}}}},
    LoadMapType{{id1, {TimeType{30}, {TimeType{30}}}},  {id2, {TimeType{100}, {TimeType{100}}}}},
    LoadMapType{{id1, {TimeType{50}, {TimeType{50}}}},  {id2, {TimeType{40}, {TimeType{40}}}}},
    LoadMapType{{id1, {TimeType{2}, {TimeType{2}}}},   {id2, {TimeType{50}, {TimeType{50}}}}},
    LoadMapType{{id1, {TimeType{60}, {TimeType{60}}}},  {id2, {TimeType{20}, {TimeType{20}}}}},
    LoadMapType{{id1, {TimeType{100}, {TimeType{100}}}}, {id2, {TimeType{10}, {TimeType{10}}}}},
  };

  for (size_t iter = 0; iter < load_holder.size(); ++iter) {
    proc_loads[iter] = load_holder[iter];
    test_model->updateLoads(iter);

    EXPECT_EQ(test_model->getNumObjects(), 2);
    EXPECT_EQ(test_model->getNumCompletedPhases(), iter+1);
    EXPECT_EQ(test_model->getNumSubphases(), 1);

    EXPECT_EQ(test_model->getNumPastPhasesNeeded(iter), iter);
    EXPECT_EQ(test_model->getNumPastPhasesNeeded(2*iter), 2*iter);

    int objects_seen = 0;
    for (auto&& obj : *test_model) {
      EXPECT_TRUE(obj.id == 1 || obj.id == 2);
      objects_seen++;

      auto work_val = test_model->getModeledLoad(
        obj, PhaseOffset{-1, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_EQ(work_val, load_holder[iter][obj].whole_phase_load);

      auto sub_work_val = test_model->getModeledLoad(obj, PhaseOffset{-1, 0});
      EXPECT_EQ(sub_work_val, load_holder[iter][obj].subphase_loads[0]);

      auto raw_load_val = test_model->getRawLoad(obj, PhaseOffset{-1, PhaseOffset::WHOLE_PHASE});
      EXPECT_EQ(raw_load_val, load_holder[iter][obj].whole_phase_load);

      auto sub_raw_load_val = test_model->getRawLoad(obj, PhaseOffset{-1, 0});
      EXPECT_EQ(sub_raw_load_val, load_holder[iter][obj].subphase_loads[0]);
    }
    EXPECT_EQ(objects_seen, 2);
  }
}

}}}} // end namespace vt::tests::unit::raw
