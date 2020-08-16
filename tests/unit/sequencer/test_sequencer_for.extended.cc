/*
//@HEADER
// *****************************************************************************
//
//                        test_sequencer_for.extended.cc
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

#include <gtest/gtest.h>

#include <cstdint>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

static constexpr vt::seq::ForIndex const end_range = 10;

struct TestSequencerFor : TestParallelHarness {
  using TestMsg = TestStaticBytesNormalMsg<4>;
  using OrderType = uint32_t;

  SEQUENCE_REGISTER_HANDLER(TestSequencerFor::TestMsg, testSeqForHan);

  static void testSeqForFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    #if DEBUG_TEST_HARNESS_PRINT
      fmt::print("testSeqForFn seq_id={}\n", seq_id);
    #endif

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 11U);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0U);

    theSeq()->for_loop(0, end_range, 1, [](vt::seq::ForIndex i) {
      theSeq()->wait_closure<TestMsg, testSeqForHan>(no_tag, [=](TestMsg* msg){
        #if DEBUG_TEST_HARNESS_PRINT
          fmt::print("testSeqForFn running wait\n");
        #endif

        EXPECT_EQ(seq_ordering_++, static_cast<size_t>(i+1));
      });
    });
  }
};

TEST_F(TestSequencerFor, test_for) {
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_seq_handler: node={}\n", my_node);
  #endif

  SeqType const& seq_id = theSeq()->nextSeq();

  runInEpochCollective([=] {
    if (my_node == 0) {
      theSeq()->sequenced(seq_id, testSeqForFn);
    }

    for (int i = 0; i < end_range; i++) {
      if (my_node == 1) {
        auto msg = makeMessage<TestMsg>();
        theMsg()->sendMsg<TestMsg, testSeqForHan>(0, msg.get());
      }
    }
  });

  if (my_node == 0) {
    testSeqForFn(-1);
  }
}

}}} // end namespace vt::tests::unit
