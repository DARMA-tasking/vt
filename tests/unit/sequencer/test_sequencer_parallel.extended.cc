/*
//@HEADER
// *****************************************************************************
//
//                     test_sequencer_parallel.extended.cc
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

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <cassert>
#include <string>
#include <sstream>

#include "test_parallel_harness.h"
#include "data_message.h"
#include "test_helpers.h"

#include "vt/sequence/sequencer.h"

namespace vt { namespace tests { namespace unit { namespace parallel {

using namespace vt;
using namespace vt::tests::unit;

using namespace std::placeholders;

#if DEBUG_TEST_HARNESS_PRINT
#define DEBUG_PRINT_SEQ_PARALLEL(ORDER, CUR, LABEL)                      \
  do {                                                                   \
    auto seq_id = theSeq()->getCurrentSeq();                             \
    fmt::print(                                                          \
      "seqParFnA ({}): seq_id={}, ordering={} -- cur={} --\n",           \
      (LABEL), seq_id, (ORDER).load(), (CUR)                             \
    );                                                                   \
  } while (false);
#else
#define DEBUG_PRINT_SEQ_PARALLEL(ORDER, CUR, LABEL)
#endif

using CountType = uint32_t;

static constexpr SeqType const SeqParFinalizeAtomicValue = -1;
static constexpr SeqType const SeqParResetAtomicValue = -2;

struct TestSequencerParallelParam : TestParallelHarnessParam<CountType> {
  using TestMsg = TestStaticBytesNormalMsg<4>;
  using OrderType = uint32_t;

  SEQUENCE_REGISTER_HANDLER(TestSequencerParallelParam::TestMsg, seqParHanN);

  void SetUp() override {
    TestParallelHarnessParam<CountType>::SetUp();
    SET_MIN_NUM_NODES_CONSTRAINT(2);
  }

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  static void waitParNum(
    std::atomic<OrderType>* order, int wait_n, int num_pars, int start_order
  ) {
    theSeq()->wait_closure<MessageT, f>(no_tag, [=](MessageT* msg){
      #if DEBUG_TEST_HARNESS_PRINT
      std::stringstream str_build;
      str_build << "PAR-";
      str_build << wait_n;
      auto str = str_build.str().c_str();
      DEBUG_PRINT_SEQ_PARALLEL(*order, wait_n, str);
      #endif
      OrderType const result = order->fetch_add(1);
      EXPECT_TRUE(
        result >= static_cast<OrderType>(start_order) or
        result <= static_cast<OrderType>(start_order + num_pars)
      );
    });
  }

  static void seqParFnN(SeqType const& seq_id, CountType const& cnt) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == SeqParResetAtomicValue or seq_id == SeqParFinalizeAtomicValue) {
      if (seq_id == SeqParResetAtomicValue) {
        seq_ordering_ = 0;
      } else {
        EXPECT_EQ(seq_ordering_++, cnt + 1);
      }
      return;
    }

    DEBUG_PRINT_SEQ_PARALLEL(seq_ordering_, 0, "INIT");
    EXPECT_EQ(seq_ordering_++, 0U);

    std::atomic<OrderType>* order_ptr = &seq_ordering_;

    constexpr CountType offset = 1;

    vt::seq::SeqFuncContainerType fns{};
    for (CountType i = 0; i < cnt; i++) {
      fns.emplace_back(
        std::bind(waitParNum<TestMsg, seqParHanN>, order_ptr, i, cnt, offset)
      );
    }

    theSeq()->parallel_lst(seq_id, fns);
  }
};

TEST_P(TestSequencerParallelParam, test_seq_parallel_param) {
  auto const& node = theContext()->getNode();

  CountType const& par_count = GetParam();

  SeqType const& seq_id = theSeq()->nextSeq();
  auto seq_par_cnt_fn = std::bind(seqParFnN, _1, par_count);

  runInEpochCollective([=]{
    if (node == 0) {
      seq_par_cnt_fn(SeqParResetAtomicValue);
      theSeq()->sequenced(seq_id, seq_par_cnt_fn);
    }

    for (CountType i = 0; i < par_count; i++) {
      if (node == 1) {
        auto msg = makeMessage<TestMsg>();
        theMsg()->sendMsg<TestMsg, seqParHanN>(0, msg);
      }
    }
  });

  if (node == 0) {
    seq_par_cnt_fn(SeqParFinalizeAtomicValue);
  }
}

INSTANTIATE_TEST_SUITE_P(
  test_seq_parallel_param, TestSequencerParallelParam,
  ::testing::Range(static_cast<CountType>(0), static_cast<CountType>(16), 1)
);

struct TestSequencerParallel : TestParallelHarness {
  using TestMsg = TestStaticBytesNormalMsg<4>;
  using OrderType = uint32_t;

  SEQUENCE_REGISTER_HANDLER(TestSequencerParallel::TestMsg, seqParHan1);
  SEQUENCE_REGISTER_HANDLER(TestSequencerParallel::TestMsg, seqParHan2);
  SEQUENCE_REGISTER_HANDLER(TestSequencerParallel::TestMsg, seqParHan3);
  SEQUENCE_REGISTER_HANDLER(TestSequencerParallel::TestMsg, seqParHan4);

  void SetUp() override {
    TestParallelHarness::SetUp();
    SET_MIN_NUM_NODES_CONSTRAINT(2);
  }

  static void seqParFn1(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 3U);
      return;
    }

    DEBUG_PRINT_SEQ_PARALLEL(seq_ordering_, 0, "INIT");

    EXPECT_EQ(seq_ordering_++, 0U);

    theSeq()->parallel(seq_id, []{
      theSeq()->wait<TestMsg, seqParHan1>([](TestMsg* msg){
        DEBUG_PRINT_SEQ_PARALLEL(seq_ordering_, 1, "PAR-1");
        auto const val = seq_ordering_.fetch_add(1);
        EXPECT_TRUE(val == 1 or val == 2);
      });
    },[]{
      theSeq()->wait<TestMsg, seqParHan1>([](TestMsg* msg){
        DEBUG_PRINT_SEQ_PARALLEL(seq_ordering_, 2, "PAR-2");
        auto const val = seq_ordering_.fetch_add(1);
        EXPECT_TRUE(val == 1 or val == 2);
      });
    });
  }

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  static void waitParNum(
    std::atomic<OrderType>* order, int wait_n, int num_pars, int start_order
  ) {
    theSeq()->wait_closure<MessageT, f>(no_tag, [=](MessageT* msg){
      #if DEBUG_TEST_HARNESS_PRINT
      std::stringstream str_build;
      str_build << "PAR-";
      str_build << wait_n;
      auto str = str_build.str().c_str();
      DEBUG_PRINT_SEQ_PARALLEL(*order, wait_n, str);
      #endif
      OrderType const result = order->fetch_add(1);
      EXPECT_TRUE(
        result >= static_cast<OrderType>(start_order) or
        result <= static_cast<OrderType>(start_order + num_pars)
      );
    });
  }

  static void seqParFn2(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 4U);
      return;
    }

    DEBUG_PRINT_SEQ_PARALLEL(seq_ordering_, 0, "INIT");

    EXPECT_EQ(seq_ordering_++, 0U);

    theSeq()->wait<TestMsg, seqParHan2>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1U);
    });

    std::atomic<OrderType>* order_ptr = &seq_ordering_;

    auto fn1 = std::bind(waitParNum<TestMsg, seqParHan2>, order_ptr, 1, 2, 2);
    auto fn2 = std::bind(waitParNum<TestMsg, seqParHan2>, order_ptr, 2, 2, 2);

    theSeq()->parallel(seq_id, fn1, fn2);
  }

  static void seqParFn3(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 5U);
      return;
    }

    DEBUG_PRINT_SEQ_PARALLEL(seq_ordering_, 0, "INIT");

    EXPECT_EQ(seq_ordering_++, 0U);

    theSeq()->wait<TestMsg, seqParHan3>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1U);
    });

    std::atomic<OrderType>* order_ptr = &seq_ordering_;

    auto fn1 = std::bind(waitParNum<TestMsg, seqParHan3>, order_ptr, 1, 2, 2);
    auto fn2 = std::bind(waitParNum<TestMsg, seqParHan3>, order_ptr, 2, 2, 2);

    theSeq()->parallel(seq_id, fn1, fn2);

    theSeq()->wait<TestMsg, seqParHan3>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 4U);
    });
  }

  static void seqParFn4(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 5U);
      return;
    }

    DEBUG_PRINT_SEQ_PARALLEL(seq_ordering_, 0, "INIT");

    EXPECT_EQ(seq_ordering_++, 0U);

    theSeq()->wait<TestMsg, seqParHan4>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1U);
    });

    theSeq()->sequenced([]{
      SeqType const my_seq_id = theSeq()->getCurrentSeq();

      std::atomic<OrderType>* order_ptr = &seq_ordering_;
      auto fn1 = std::bind(waitParNum<TestMsg, seqParHan4>, order_ptr, 1, 2, 2);
      auto fn2 = std::bind(waitParNum<TestMsg, seqParHan4>, order_ptr, 2, 2, 2);

      theSeq()->parallel(my_seq_id, fn1, fn2);
    });

    theSeq()->wait<TestMsg, seqParHan4>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 4U);
    });
  }
};

#define PAR_EXPAND(SEQ_HAN, SEQ_FN, NODE, MSG_TYPE, NUM_MSGS, IS_TAG) \
  do {                                                                \
    SeqType const& seq_id = theSeq()->nextSeq();                      \
    runInEpochCollective([=]{                                         \
      if ((NODE) == 0) {                                              \
        theSeq()->sequenced(seq_id, (SEQ_FN));                        \
      }                                                               \
      for (int i = 0; i < (NUM_MSGS); i++) {                          \
        TagType const tag = (IS_TAG) ? i+1 : no_tag;                  \
        if ((NODE) == 1) {                                            \
          auto msg = makeMessage<MSG_TYPE>();                         \
          theMsg()->sendMsg<MSG_TYPE, SEQ_HAN>(                       \
            0, msg, tag                                               \
          );                                                          \
        }                                                             \
      }                                                               \
    });                                                               \
    if ((NODE) == 0) {                                                \
        SEQ_FN(-1);                                                   \
    }                                                                 \
  } while (false);


TEST_F(TestSequencerParallel, test_parallel_1) {
  auto const& node = theContext()->getNode();
  PAR_EXPAND(seqParHan1, seqParFn1, node, TestMsg, 2, false);
}

TEST_F(TestSequencerParallel, test_parallel_2) {
  auto const& node = theContext()->getNode();
  PAR_EXPAND(seqParHan2, seqParFn2, node, TestMsg, 3, false);
}

TEST_F(TestSequencerParallel, test_parallel_3) {
  auto const& node = theContext()->getNode();
  PAR_EXPAND(seqParHan3, seqParFn3, node, TestMsg, 4, false);
}

TEST_F(TestSequencerParallel, test_parallel_4) {
  auto const& node = theContext()->getNode();
  PAR_EXPAND(seqParHan4, seqParFn4, node, TestMsg, 4, false);
}

}}}} // end namespace vt::tests::unit::parallel
