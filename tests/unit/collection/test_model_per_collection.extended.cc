/*
//@HEADER
// *****************************************************************************
//
//                    test_model_per_collection.extended.cc
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

#include <vt/vrt/collection/balance/model/per_collection.h>
#include <vt/vrt/collection/balance/model/composed_model.h>
#include <vt/vrt/collection/balance/model/load_model.h>
#include <vt/vrt/collection/manager.h>

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace per {

struct TestCol1 : vt::Collection<TestCol1,vt::Index1D> { };
struct TestCol2 : vt::Collection<TestCol2,vt::Index1D> { };

using TestModelPerCollection = TestParallelHarness;

static constexpr int32_t const num_elms = 64;

using vt::vrt::collection::balance::ComposedModel;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::ElementIDStruct;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::PerCollection;

struct ConstantTestModel : ComposedModel {

  ConstantTestModel(std::shared_ptr<LoadModel> in_base, VirtualProxyType in_proxy)
    : vt::vrt::collection::balance::ComposedModel(in_base),
      proxy_(in_proxy)
  { }

  TimeType getModeledLoad(ElementIDStruct, PhaseOffset) const override {
    return static_cast<TimeType>(proxy_);
  }

private:
  VirtualProxyType proxy_ = 0;
};

std::unordered_map<ElementIDStruct, VirtualProxyType> id_proxy_map;

template <typename ColT>
void colHandler(ColT* col) {
  // do nothing, except setting up our map using the temp ID, which will hit
  // every node
  id_proxy_map[col->getElmID()] = col->getProxy();
}

TEST_F(TestModelPerCollection, test_model_per_collection_1) {
  id_proxy_map = {};

  // We must have more or equal number of elements than nodes for this test to
  // work properly
  EXPECT_GE(num_elms, vt::theContext()->getNumNodes());

  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<TestCol1> proxy1;
  vt::vrt::collection::CollectionProxy<TestCol2> proxy2;

  // Construct two collections
  runInEpochCollective([&]{
    proxy1 = vt::theCollection()->constructCollective<TestCol1>(
      range, "test_model_per_collection_1"
    );
    proxy2 = vt::theCollection()->constructCollective<TestCol2>(
      range, "test_model_per_collection_1"
    );
  });

  // Get the base model, assert it's valid
  auto base = theLBManager()->getBaseLoadModel();
  EXPECT_NE(base, nullptr);
  EXPECT_TRUE(base->hasRawLoad());

  // Create a new PerCollection model
  auto per_col = std::make_shared<PerCollection>(base);

  // Add two distinct models for each collection that return the proxy for the
  // amount of work
  auto proxy1_untyped = proxy1.getProxy();
  auto proxy2_untyped = proxy2.getProxy();
  per_col->addModel(
    proxy1_untyped, std::make_shared<ConstantTestModel>(base, proxy1_untyped)
  );
  per_col->addModel(
    proxy2_untyped, std::make_shared<ConstantTestModel>(base, proxy2_untyped)
  );
  EXPECT_TRUE(per_col->hasRawLoad());

  // Set the new model
  theLBManager()->setLoadModel(per_col);

  // Do some work.
  runInEpochCollective([&]{
    proxy1.broadcastCollective<colHandler<TestCol1>>();
    proxy2.broadcastCollective<colHandler<TestCol2>>();
  });

  // Add a hook for after LB runs, but before instrumentation is cleared
  thePhase()->registerHookCollective(phase::PhaseHook::End, [=]{
    // LB control flow means that there will be no recorded phase for
    // this to even look up objects in, causing failure
#if vt_check_enabled(lblite)
    // Test the model, which should be per-collection and return the proxy.
    auto model = theLBManager()->getLoadModel();
    // Call updateLoads manually, since it won't be called by the LB
    // infrastructure when the LB hasn't run, and we need this for the
    // model to function
    model->updateLoads(0);
    for (auto&& obj : *model) {
      auto work_val = model->getModeledLoad(
        obj, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      if (id_proxy_map.find(obj) != id_proxy_map.end()) {
        EXPECT_DOUBLE_EQ(work_val, static_cast<TimeType>(id_proxy_map[obj]));
      }
      if (model->hasRawLoad()) {
        auto raw_load_val = model->getRawLoad(obj, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE});
        if (id_proxy_map.find(obj) != id_proxy_map.end()) {
          EXPECT_NE(raw_load_val, work_val);
        }
      }
      //fmt::print("{:x} {}\n", obj, work_val);
    }
#endif
  });

  // Go to the next phase.
  vt::thePhase()->nextPhaseCollective();

}

}}}} // end namespace vt::tests::unit::per
