/*
//@HEADER
// *****************************************************************************
//
//                         test_garbage_collection.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

struct TestGarbageCollection : TestParallelHarness {
  // Inject arguments to set the garbage collection threshold very low for
  // testing the protocol. Otherwise, it would never engage unless billions of
  // epoch are processed.
  void addAdditionalArgs() override {
    static char gc_threshold[]{"--vt_term_gc_threshold=0.000000001"};
    addArgs(gc_threshold);
  }
};

// Start with testing garbage collection for epochs that terminate sequentially,
// testing that garbage collection actually runs with the set threshold and
// frees the epochs properly.
TEST_F(TestGarbageCollection, test_garbage_collecton_1) {
  // (1<<41)*.0000000010 = ~2,199 collective epochs to process per GC

  auto const num_bits = vt::epoch::epoch_seq_coll_num_bits;
  auto ep_per_gc = static_cast<uint64_t>(((1ull << num_bits) * 0.000000001) + 0.5);
  auto num_iter = 10000ull;

  fmt::print("num bits={}\n", num_bits);
  fmt::print("num epochs={}\n", 1ull << (int64_t)vt::epoch::epoch_seq_coll_num_bits);
  fmt::print("ep_per_gc={}\n", ep_per_gc);

  for (uint64_t i = 0; i < num_iter; i++) {
    vt::runInEpochCollective([]{});
  }

  auto w = theEpoch()->getTerminatedWindow(epoch::EpochManip::generateEpoch());
  auto const& free = w->getFreeSet();
  auto const& term = w->getTerminatedSet();
  auto const& total = w->getRangeSize();

  auto const expected_gcs = num_iter / ep_per_gc;
  auto const expected_free = ep_per_gc * expected_gcs;

  fmt::print("term.size() = {}\n", term.size());
  fmt::print("total - free.size() = {}\n", total - free.size());
  fmt::print("expected_free = {}\n", expected_free);

  EXPECT_TRUE(num_iter - expected_free >= term.size());
  EXPECT_TRUE(total - free.size() < (num_iter - expected_free)*1.1);
}

}}} // end namespace vt::tests::unit
