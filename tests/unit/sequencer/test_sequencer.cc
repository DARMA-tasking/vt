/*
//@HEADER
// ************************************************************************
//
//                          test_sequencer.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
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

struct TestSequencer : TestParallelHarness {
  using TestMsg = TestStaticBytesNormalMsg<4>;
  using OrderType = uint32_t;

  static TagType single_tag;
  static TagType single_tag_2;

  virtual void SetUp() {
    TestParallelHarness::SetUp();

    TestSequencer::single_tag = 29;
    TestSequencer::single_tag_2 = 31;
  }

  SEQUENCE_REGISTER_HANDLER(TestSequencer::TestMsg, testSeqHan);
  SEQUENCE_REGISTER_HANDLER(TestSequencer::TestMsg, testSeqTaggedHan);
  SEQUENCE_REGISTER_HANDLER(TestSequencer::TestMsg, testSeqMultiHan);
  SEQUENCE_REGISTER_HANDLER(TestSequencer::TestMsg, testSeqMultiTaggedHan);

  static void testSingleWaitFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    #if DEBUG_TEST_HARNESS_PRINT
      fmt::print("testSingleWaitFn seq_id={}\n", seq_id);
    #endif

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 2U);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0U);

    theSeq()->wait<TestMsg, testSeqHan>([](TestMsg* msg){
      #if DEBUG_TEST_HARNESS_PRINT
        fmt::print("testSingleWaitFn running wait\n");
      #endif

      EXPECT_EQ(seq_ordering_++, 1U);
    });
  }

  static void testSingleTaggedWaitFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 2U);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0U);

    theSeq()->wait<TestMsg, testSeqTaggedHan>(single_tag, [](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1U);
    });
  }

  static void testMultiWaitFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 3U);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0U);

    theSeq()->wait<TestMsg, testSeqMultiHan>([](TestMsg* msg){
      EXPECT_TRUE(seq_ordering_ == 1 or seq_ordering_ == 2);
      seq_ordering_++;
    });
    theSeq()->wait<TestMsg, testSeqMultiHan>([](TestMsg* msg){
      EXPECT_TRUE(seq_ordering_ == 1 or seq_ordering_ == 2);
      seq_ordering_++;
    });
  }

  static void testMultiTaggedWaitFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 3U);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0U);

    theSeq()->wait<TestMsg, testSeqMultiTaggedHan>(single_tag, [](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1U);
    });
    theSeq()->wait<TestMsg, testSeqMultiTaggedHan>(single_tag_2, [](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 2U);
    });
  }
};

/*static*/ TagType TestSequencer::single_tag;
/*static*/ TagType TestSequencer::single_tag_2;

TEST_F(TestSequencer, test_single_wait) {
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_seq_handler: node={}\n", my_node);
  #endif

  if (my_node == 0) {
    SeqType const& seq_id = theSeq()->nextSeq();
    theSeq()->sequenced(seq_id, testSingleWaitFn);

    theMsg()->sendMsg<TestMsg, testSeqHan>(my_node, makeSharedMessage<TestMsg>());

    theTerm()->addAction([=]{
      testSingleWaitFn(-1);
    });
  }
}

TEST_F(TestSequencer, test_single_wait_tagged) {
  auto const& my_node = theContext()->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq()->nextSeq();
    theSeq()->sequenced(seq_id, testSingleTaggedWaitFn);

    theMsg()->sendMsg<TestMsg, testSeqTaggedHan>(
      my_node, makeSharedMessage<TestMsg>(), single_tag
    );

    theTerm()->addAction([=]{
      testSingleTaggedWaitFn(-1);
    });
  }
}

TEST_F(TestSequencer, test_multi_wait) {
  auto const& my_node = theContext()->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq()->nextSeq();
    theSeq()->sequenced(seq_id, testMultiWaitFn);

    theMsg()->sendMsg<TestMsg, testSeqMultiHan>(
      my_node, makeSharedMessage<TestMsg>()
    );
    theMsg()->sendMsg<TestMsg, testSeqMultiHan>(
      my_node, makeSharedMessage<TestMsg>()
    );

    theTerm()->addAction([=]{
      testMultiWaitFn(-1);
    });
  }
}

TEST_F(TestSequencer, test_multi_wait_tagged) {
  auto const& my_node = theContext()->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq()->nextSeq();
    theSeq()->sequenced(seq_id, testMultiTaggedWaitFn);

    theMsg()->sendMsg<TestMsg, testSeqMultiTaggedHan>(
      my_node, makeSharedMessage<TestMsg>(), single_tag
    );
    theMsg()->sendMsg<TestMsg, testSeqMultiTaggedHan>(
      my_node, makeSharedMessage<TestMsg>(), single_tag_2
    );

    theTerm()->addAction([=]{
      testMultiTaggedWaitFn(-1);
    });
  }
}

}}} // end namespace vt::tests::unit
