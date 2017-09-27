
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

#if DEBUG_TEST_HARNESS_PRINT
#define DEBUG_PRINT_SEQ_NESTED(ORDER, CUR)                              \
  do {                                                                  \
    auto seq_id = theSeq->getCurrentSeq();                              \
    printf(                                                             \
      "testSeqDeepNested: seq_id=%d, ordering=%d -- cur=%d --\n",       \
      seq_id, (ORDER).load(), (CUR)                                     \
    );                                                                  \
  } while (false);
#else
#define DEBUG_PRINT_SEQ_NESTED(ORDER, CUR)
#endif

struct TestSequencerNested : TestParallelHarness {
  using TestMsg = TestStaticBytesNormalMsg<4>;
  using OrderType = uint32_t;

  static TagType single_tag;
  static TagType single_tag_2;

  virtual void SetUp() {
    TestParallelHarness::SetUp();

    TestSequencerNested::single_tag = 29;
    TestSequencerNested::single_tag_2 = 31;
  }

  SEQUENCE_REGISTER_HANDLER(TestSequencerNested::TestMsg, testSeqNested);
  SEQUENCE_REGISTER_HANDLER(TestSequencerNested::TestMsg, testSeqNestedMulti);
  SEQUENCE_REGISTER_HANDLER(TestSequencerNested::TestMsg, testSeqNestedSingleHan);
  SEQUENCE_REGISTER_HANDLER(TestSequencerNested::TestMsg, testSeqNestedSingle2Han);
  SEQUENCE_REGISTER_HANDLER(TestSequencerNested::TestMsg, testSeqDeepNestedHan);

  static void testNestedWaitFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 2);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->sequenced([]{
      theSeq->wait<TestMsg, testSeqNested>([](TestMsg* msg){
        EXPECT_EQ(seq_ordering_++, 1);
      });
    });
  }

  static void testNestedWaitFnMulti(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 3);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, testSeqNestedMulti>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1);
    });

    theSeq->sequenced([]{
      theSeq->wait<TestMsg, testSeqNestedMulti>([](TestMsg* msg){
        EXPECT_EQ(seq_ordering_++, 2);
      });
    });
  }

  static void testNestedSingle(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 4);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, testSeqNestedSingleHan>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1);
    });

    theSeq->sequenced([]{
      theSeq->sequenced([]{
        theSeq->sequenced([]{
          theSeq->wait<TestMsg, testSeqNestedSingleHan>([](TestMsg* msg){
            EXPECT_EQ(seq_ordering_++, 2);
          });
        });
      });
    });

    theSeq->wait<TestMsg, testSeqNestedSingleHan>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 3);
    });
  }

  static void testNestedSingle2(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 5);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, testSeqNestedSingle2Han>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1);
    });

    theSeq->sequenced([]{
      theSeq->sequenced([]{
        theSeq->sequenced([]{
          theSeq->wait<TestMsg, testSeqNestedSingle2Han>([](TestMsg* msg){
            EXPECT_EQ(seq_ordering_++, 2);
          });
        });
      });
    });

    theSeq->sequenced([]{
      theSeq->sequenced([]{
        theSeq->sequenced([]{
          theSeq->wait<TestMsg, testSeqNestedSingle2Han>([](TestMsg* msg){
            EXPECT_EQ(seq_ordering_++, 3);
          });
        });
      });
    });

    theSeq->wait<TestMsg, testSeqNestedSingle2Han>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 4);
    });
  }

  static void testSeqDeepNested(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 9);
      return;
    }

    #if DEBUG_TEST_HARNESS_PRINT
      printf("testSeqDeepNested: seq_id=%d, ordering=%d\n", seq_id, seq_ordering_.load());
    #endif

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, testSeqDeepNestedHan>(1, [](TestMsg* msg){
      DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 1);
      EXPECT_EQ(seq_ordering_++, 1);
    });

    theSeq->sequenced([]{
      theSeq->wait<TestMsg, testSeqDeepNestedHan>(2, [](TestMsg* msg){
        DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 2);
        EXPECT_EQ(seq_ordering_++, 2);
      });

      theSeq->wait<TestMsg, testSeqDeepNestedHan>(3, [](TestMsg* msg){
        DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 3);
        EXPECT_EQ(seq_ordering_++, 3);
      });

      theSeq->sequenced([]{
        theSeq->wait<TestMsg, testSeqDeepNestedHan>(4, [](TestMsg* msg){
          DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 4);
          EXPECT_EQ(seq_ordering_++, 4);
        });

        theSeq->sequenced([]{
          theSeq->wait<TestMsg, testSeqDeepNestedHan>(5, [](TestMsg* msg){
            DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 5);
            EXPECT_EQ(seq_ordering_++, 5);
          });
        });

        theSeq->wait<TestMsg, testSeqDeepNestedHan>(6, [](TestMsg* msg){
          DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 6);
          EXPECT_EQ(seq_ordering_++, 6);
        });
      });

      theSeq->wait<TestMsg, testSeqDeepNestedHan>(7, [](TestMsg* msg){
        DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 7);
        EXPECT_EQ(seq_ordering_++, 7);
      });
    });

    theSeq->wait<TestMsg, testSeqDeepNestedHan>(8, [](TestMsg* msg){
      DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 8);
      EXPECT_EQ(seq_ordering_++, 8);
    });
  }

};

/*static*/ TagType TestSequencerNested::single_tag;
/*static*/ TagType TestSequencerNested::single_tag_2;

#define SEQ_TEST(SEQ_HAN, SEQ_FN, NODE, MSG_TYPE, NUM_MSGS, IS_TAG)   \
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

TEST_F(TestSequencerNested, test_simple_nested_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SEQ_TEST(testSeqNested, testNestedWaitFn, my_node, TestMsg, 1, false);
  }
}

TEST_F(TestSequencerNested, test_multi_nested_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SEQ_TEST(
      testSeqNestedMulti, testNestedWaitFnMulti, my_node, TestMsg, 2, false
    );
  }
}

TEST_F(TestSequencerNested, test_multi_nested_single_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SEQ_TEST(
      testSeqNestedSingleHan, testNestedSingle, my_node, TestMsg, 3, false
    );
  }
}

TEST_F(TestSequencerNested, test_multi_nested_single2_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SEQ_TEST(
      testSeqNestedSingle2Han, testNestedSingle2, my_node, TestMsg, 4, false
    );
  }
}

TEST_F(TestSequencerNested, test_multi_deep_nested_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SEQ_TEST(
      testSeqDeepNestedHan, testSeqDeepNested, my_node, TestMsg, 8, true
    );
  }
}

}}} // end namespace vt::tests::unit
