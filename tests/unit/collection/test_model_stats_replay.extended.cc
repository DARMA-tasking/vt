/*
//@HEADER
// *****************************************************************************
//
//                    test_model_stats_replay.extended.cc
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

#include <vt/vrt/collection/balance/model/load_model.h>
#include <vt/vrt/collection/balance/load_stats_replayer.h>
#include <vt/vrt/collection/manager.h>

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace replay {

using TestModelStatsReplay = TestParallelHarness;

using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::LoadStatsReplayer;

TEST_F(TestModelStatsReplay, test_model_stats_replay_1) {
  std::size_t coll_elms_per_node = 3;
  std::size_t initial_phase = 3;
  vt::theLoadStatsReplayer()->createCollectionAndModel(
    coll_elms_per_node, initial_phase
  );

  auto this_node = vt::theContext()->getNode();
  LoadStatsReplayer::ElmPhaseLoadsMapType loads;
  for (std::size_t y=0; y<coll_elms_per_node; ++y) {
    auto elm_id = (y << 32) | this_node;
    loads[elm_id][initial_phase] = y + this_node + 2;
  }

  vt::theLoadStatsReplayer()->configureCollectionForReplay(
    loads, initial_phase
  );

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
      auto returned_load = model->getWork(
        obj, {PhaseOffset::NEXT_PHASE, PhaseOffset::WHOLE_PHASE}
      );
      EXPECT_DOUBLE_EQ(returned_load, loads.at(obj.id).at(initial_phase));
    }
#endif
  });

  // Go to the next phase.
  vt::thePhase()->nextPhaseCollective();
}

}}}} // end namespace vt::tests::unit::replay
