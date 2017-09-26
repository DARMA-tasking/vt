
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <cassert>
#include <string>
#include <sstream>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

using namespace vt;
using namespace vt::tests::unit;

using namespace std::placeholders;

#if DEBUG_TEST_HARNESS_PRINT
#define DEBUG_PRINT_SEQ_NESTED(ORDER, CUR, LABEL)                       \
  do {                                                                  \
    auto seq_id = theSeq->getCurrentSeq();                              \
    printf(                                                             \
      "seqParFnA (%s): seq_id=%d, ordering=%d -- cur=%d --\n",   \
      (LABEL), seq_id, (ORDER).load(), (CUR)                            \
    );                                                                  \
  } while (false);
#else
#define DEBUG_PRINT_SEQ_NESTED(ORDER, CUR, LABEL)
#endif

using CountType = uint32_t;

static constexpr SeqType const SeqParFinalizeAtomicValue = -1;
static constexpr SeqType const SeqParResetAtomicValue = -2;

struct TestSequencerParallelParam : TestParallelHarnessParam<CountType> {
  using TestMsg = TestStaticBytesNormalMsg<4>;
  using OrderType = uint32_t;

  SEQUENCE_REGISTER_HANDLER(TestSequencerParallelParam::TestMsg, seqParHanN);

  virtual void SetUp() {
    TestParallelHarnessParam<CountType>::SetUp();
  }

  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  static void waitParNum(
    std::atomic<OrderType>* order, int wait_n, int num_pars, int start_order
  ) {
    theSeq->wait_closure<MessageT, f>(no_tag, [=](MessageT* msg){
      std::stringstream str_build;
      str_build << "PAR-";
      str_build << wait_n;
      auto str = str_build.str().c_str();
      DEBUG_PRINT_SEQ_NESTED(*order, wait_n, str);
      OrderType const result = order->fetch_add(1);
      EXPECT_TRUE(result >= start_order or result <= start_order + num_pars);
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

    DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 0, "INIT");
    EXPECT_EQ(seq_ordering_++, 0);

    std::atomic<OrderType>* order_ptr = &seq_ordering_;

    constexpr CountType offset = 1;

    vt::seq::SeqFuncContainerType fns{};
    for (CountType i = 0; i < cnt; i++) {
      fns.emplace_back(
        std::bind(waitParNum<TestMsg, seqParHanN>, order_ptr, i, cnt, offset)
      );
    }

    theSeq->parallel_lst(seq_id, fns);
  }
};

TEST_P(TestSequencerParallelParam, test_seq_parallel_param) {
  auto const& node = theContext->getNode();

  CountType const& par_count = GetParam();

  if (node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();

    auto seq_par_cnt_fn = std::bind(seqParFnN, _1, par_count);
    seq_par_cnt_fn(SeqParResetAtomicValue);

    theSeq->sequenced(seq_id, seq_par_cnt_fn);
    for (CountType i = 0; i < par_count; i++) {
      theMsg->sendMsg<TestMsg, seqParHanN>(node, makeSharedMessage<TestMsg>());
    }
    theTerm->attachGlobalTermAction([=]{
      seq_par_cnt_fn(SeqParFinalizeAtomicValue);
    });
  }
}

INSTANTIATE_TEST_CASE_P(
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

  static void seqParFn1(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 3);
      return;
    }

    DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 0, "INIT");

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->parallel(seq_id, []{
      theSeq->wait<TestMsg, seqParHan1>([](TestMsg* msg){
        DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 1, "PAR-1");
        auto const val = seq_ordering_.fetch_add(1);
        EXPECT_TRUE(val == 1 or val == 2);
      });
    },[]{
      theSeq->wait<TestMsg, seqParHan1>([](TestMsg* msg){
        DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 2, "PAR-2");
        auto const val = seq_ordering_.fetch_add(1);
        EXPECT_TRUE(val == 1 or val == 2);
      });
    });
  }

  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  static void waitParNum(
    std::atomic<OrderType>* order, int wait_n, int num_pars, int start_order
  ) {
    theSeq->wait_closure<MessageT, f>(no_tag, [=](MessageT* msg){
      std::stringstream str_build;
      str_build << "PAR-";
      str_build << wait_n;
      auto str = str_build.str().c_str();
      DEBUG_PRINT_SEQ_NESTED(*order, wait_n, str);
      OrderType const result = order->fetch_add(1);
      EXPECT_TRUE(result >= start_order or result <= start_order + num_pars);
    });
  }

  static void seqParFn2(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 4);
      return;
    }

    DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 0, "INIT");

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, seqParHan2>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1);
    });

    std::atomic<OrderType>* order_ptr = &seq_ordering_;

    auto fn1 = std::bind(waitParNum<TestMsg, seqParHan2>, order_ptr, 1, 2, 2);
    auto fn2 = std::bind(waitParNum<TestMsg, seqParHan2>, order_ptr, 2, 2, 2);

    theSeq->parallel(seq_id, fn1, fn2);
  }

  static void seqParFn3(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 5);
      return;
    }

    DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 0, "INIT");

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, seqParHan3>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1);
    });

    std::atomic<OrderType>* order_ptr = &seq_ordering_;

    auto fn1 = std::bind(waitParNum<TestMsg, seqParHan3>, order_ptr, 1, 2, 2);
    auto fn2 = std::bind(waitParNum<TestMsg, seqParHan3>, order_ptr, 2, 2, 2);

    theSeq->parallel(seq_id, fn1, fn2);

    theSeq->wait<TestMsg, seqParHan3>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 4);
    });
  }

  static void seqParFn4(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 5);
      return;
    }

    DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 0, "INIT");

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, seqParHan4>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1);
    });

    theSeq->sequenced([]{
      SeqType const seq_id = theSeq->getCurrentSeq();

      std::atomic<OrderType>* order_ptr = &seq_ordering_;
      auto fn1 = std::bind(waitParNum<TestMsg, seqParHan4>, order_ptr, 1, 2, 2);
      auto fn2 = std::bind(waitParNum<TestMsg, seqParHan4>, order_ptr, 2, 2, 2);

      theSeq->parallel(seq_id, fn1, fn2);
    });

    theSeq->wait<TestMsg, seqParHan4>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 4);
    });
  }
};

#define PAR_TEST(SEQ_HAN, SEQ_FN, NODE, MSG_TYPE, NUM_MSGS, IS_TAG)   \
  do {                                                                \
    SeqType const& seq_id = theSeq->nextSeq();                        \
    theSeq->sequenced(seq_id, (SEQ_FN));                              \
    for (int i = 0; i < (NUM_MSGS); i++) {                            \
      TagType const tag = (IS_TAG) ? i+1 : no_tag;                    \
      theMsg->sendMsg<MSG_TYPE, SEQ_HAN>(                             \
        (NODE), makeSharedMessage<MSG_TYPE>(), tag                    \
      );                                                              \
    }                                                                 \
    theTerm->attachGlobalTermAction([=]{                              \
      SEQ_FN(-1);                                                     \
    });                                                               \
  } while (false);

TEST_F(TestSequencerParallel, test_parallel_1) {
  auto const& node = theContext->getNode();

  if (node == 0) {
    PAR_TEST(seqParHan1, seqParFn1, node, TestMsg, 2, false);
  }
}

TEST_F(TestSequencerParallel, test_parallel_2) {
  auto const& node = theContext->getNode();

  if (node == 0) {
    PAR_TEST(seqParHan2, seqParFn2, node, TestMsg, 3, false);
  }
}

TEST_F(TestSequencerParallel, test_parallel_3) {
  auto const& node = theContext->getNode();
  if (node == 0) {
    PAR_TEST(seqParHan3, seqParFn3, node, TestMsg, 4, false);
  }
}

TEST_F(TestSequencerParallel, test_parallel_4) {
  auto const& node = theContext->getNode();
  if (node == 0) {
    PAR_TEST(seqParHan4, seqParFn4, node, TestMsg, 4, false);
  }
}
