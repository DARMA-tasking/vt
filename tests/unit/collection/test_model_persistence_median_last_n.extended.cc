/*
//@HEADER
// *****************************************************************************
//
//                 test_model_persistence_median_last_n.extended
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
#include <vt/vrt/collection/balance/model/composed_model.h>
#include <vt/vrt/collection/balance/model/load_model.h>
#include <vt/vrt/collection/balance/model/persistence_median_last_n.h>

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit {

struct TestCol : vt::Collection<TestCol, vt::Index1D> {};

using TestModelPersistenceMedianLastN = TestParallelHarness;

static constexpr int32_t const num_elms = 64;
static constexpr int32_t const num_iter = 4;
static constexpr std::array<TimeType, num_iter> const work_arr = {
  10, 4, 20, 10};

using vt::vrt::collection::balance::ComposedModel;
using vt::vrt::collection::balance::ElementIDType;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::PersistenceMedianLastN;
using vt::vrt::collection::balance::PhaseOffset;

struct ConstantTestModel : ComposedModel {

  ConstantTestModel(std::shared_ptr<LoadModel> in_base)
    : vt::vrt::collection::balance::ComposedModel(in_base) {}

  TimeType getWork(ElementIDType id, PhaseOffset phase) override {
    return work_arr.at(-1 * phase.phases - 1);
  }
};

TEST_F(
  TestModelPersistenceMedianLastN, test_model_persistence_median_last_n_1) {
  // We must have more or equal number of elements than nodes for this test to
  // work properly
  EXPECT_GE(num_elms, vt::theContext()->getNumNodes());

  auto range = vt::Index1D(num_elms);
  auto proxy = vt::theCollection()->constructCollective<TestCol>(
    range, [](vt::Index1D){ return std::make_unique<TestCol>(); });

  // Get the base model, assert it's valid
  auto base = theLBManager()->getBaseLoadModel();
  EXPECT_NE(base, nullptr);

  // Create a new PersistenceMedianLastN model
  auto test_model = std::make_shared<PersistenceMedianLastN>(
    std::make_shared<ConstantTestModel>(base), num_iter);

  // Set the new model
  theLBManager()->setLoadModel(test_model);

  // Generate 'num_iter' phases
  runInEpochCollective([phases = num_iter]{
    for (int i = 0; i < phases; ++i)
      vt::theCollection()->startPhaseCollective(nullptr);
  });

  // LB control flow means that there will be no recorded phase for
  // this to even look up objects in, causing failure
#if vt_check_enabled(lblite)
  auto model = theLBManager()->getLoadModel();
  EXPECT_NE(model, nullptr);
  for (auto&& obj : *model) {
    auto work_val = model->getWork(obj, PhaseOffset{});
    EXPECT_EQ(work_val, TimeType{10});
  }
#endif
}

}}} // end namespace vt::tests::unit
