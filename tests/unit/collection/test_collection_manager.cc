/*
//@HEADER
// *****************************************************************************
//
//                          test_collection_manager.cc
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

#include <vt/vrt/collection/manager.h>

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "test_helpers.h"

#include <memory>

namespace vt { namespace tests { namespace unit {

struct TestCollectionManager : TestParallelHarness {};

struct TestCol : vt::Collection<TestCol,vt::Index1D> {
  static void colHandler(TestCol*) {}
};

static constexpr int32_t const num_elms = 16;
static constexpr int const num_phases = 5;

TEST_F(TestCollectionManager, test_collection_manager_proxy_deletion) {
  auto range = vt::Index1D(num_elms);

  vt::vrt::collection::CollectionProxy<TestCol> proxy;

  // Construct collection
  runInEpochCollective([&]{
    proxy = vt::theCollection()->constructCollective<TestCol>(
      range, "test_collection_manager_proxy_deletion"
    );
  });

  for (int i=0; i<num_phases; ++i) {
    runInEpochCollective([&]{
      // Do some work.
      proxy.broadcastCollective<TestCol::colHandler>();
    });

    // Remove proxy prematurely
    if (i == (num_phases - 1)) {
        vt::theCollection()->destroy(proxy);
    }

    // Go to the next phase.
    vt::thePhase()->nextPhaseCollective();
  }
}

}}} // end namespace vt::tests::unit
