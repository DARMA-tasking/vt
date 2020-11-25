/*
//@HEADER
// *****************************************************************************
//
//                    test_model_naive_persistence.nompi.cc
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
#include <vt/vrt/collection/balance/model/naive_persistence.h>

#include <gtest/gtest.h>

#include "test_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace naive {

using TestModelNaivePersistence = TestHarness;

using vt::vrt::collection::balance::ElementIDStruct;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::NaivePersistence;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::LoadMapType;
using vt::vrt::collection::balance::SubphaseLoadMapType;
using vt::vrt::collection::balance::CommMapType;
using vt::vrt::collection::balance::ObjectIterator;

static int32_t getIndexFromPhase(int32_t phase) {
  return std::max(0, -1 * phase - 1);
}

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
    EXPECT_LE(phase.phases, -1);
    return proc_load_->at(getIndexFromPhase(phase.phases)).at(id);
  }

  virtual ObjectIterator begin() override {
    return ObjectIterator(proc_load_->at(3).begin());
  }
  virtual ObjectIterator end() override {
    return ObjectIterator(proc_load_->at(3).end());
  }

  // Not used in this test
  virtual int getNumObjects() override { return 1; }
  virtual unsigned int getNumCompletedPhases() override { return 1; }
  virtual int getNumSubphases() override { return 1; }
  unsigned int getNumPastPhasesNeeded(unsigned int look_back = 0) override { return look_back; }

private:
  std::unordered_map<PhaseType, LoadMapType> const* proc_load_ = nullptr;
};

TEST_F(TestModelNaivePersistence, test_model_naive_persistence_1) {
  NodeType this_node = 0;
  std::unordered_map<PhaseType, LoadMapType> proc_loads = {
    {0, LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{10}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{40}}}},
    {1, LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{4}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{10}}}},
    {2, LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{20}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{50}}}},
    {3, LoadMapType{
      {ElementIDStruct{1,this_node,this_node}, TimeType{40}},
      {ElementIDStruct{2,this_node,this_node}, TimeType{100}}}}};

  auto test_model =
    std::make_shared<NaivePersistence>(std::make_shared<StubModel>());

  test_model->setLoads(&proc_loads, nullptr, nullptr);
  test_model->updateLoads(3);

  for (auto&& obj : *test_model) {
    for (auto phase : {0, -1, -2, -3, -4}) {
      auto work_val = test_model->getWork(obj, PhaseOffset{phase, 1});
      EXPECT_EQ(work_val, proc_loads.at(getIndexFromPhase(phase)).at(obj));
    }
  }
}

}}}} // end namespace vt::tests::unit::naive
