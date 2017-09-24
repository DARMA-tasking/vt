
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

using namespace vt;
using namespace vt::tests::unit;

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

  static void testSeqDeepNested(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 7);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, testSeqDeepNestedHan>([](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1);
    });

    theSeq->sequenced([]{
      theSeq->wait<TestMsg, testSeqDeepNestedHan>([](TestMsg* msg){
        EXPECT_EQ(seq_ordering_++, 2);
      });

      theSeq->sequenced([]{
        theSeq->wait<TestMsg, testSeqDeepNestedHan>([](TestMsg* msg){
          EXPECT_EQ(seq_ordering_++, 3);
        });

        theSeq->sequenced([]{
          theSeq->wait<TestMsg, testSeqDeepNestedHan>([](TestMsg* msg){
            EXPECT_EQ(seq_ordering_++, 4);
          });
        });

        theSeq->wait<TestMsg, testSeqDeepNestedHan>([](TestMsg* msg){
          EXPECT_EQ(seq_ordering_++, 5);
        });
      });

      theSeq->wait<TestMsg, testSeqDeepNestedHan>([](TestMsg* msg){
        EXPECT_EQ(seq_ordering_++, 6);
      });
    });

    // theSeq->wait<TestMsg, testSeqDeepNestedHan>([](TestMsg* msg){
    //   EXPECT_EQ(seq_ordering_++, 7);
    // });
  }

};

/*static*/ TagType TestSequencerNested::single_tag;
/*static*/ TagType TestSequencerNested::single_tag_2;

TEST_F(TestSequencerNested, test_simple_nested_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, testNestedWaitFn);
    theMsg->sendMsg<TestMsg, testSeqNested>(
      my_node, makeSharedMessage<TestMsg>()
    );
    theTerm->attachGlobalTermAction([=]{
      testNestedWaitFn(-1);
    });
  }
}

TEST_F(TestSequencerNested, test_multi_nested_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, testNestedWaitFnMulti);
    theMsg->sendMsg<TestMsg, testSeqNestedMulti>(
      my_node, makeSharedMessage<TestMsg>()
    );
    theMsg->sendMsg<TestMsg, testSeqNestedMulti>(
      my_node, makeSharedMessage<TestMsg>()
    );
    theTerm->attachGlobalTermAction([=]{
      testNestedWaitFnMulti(-1);
    });
  }
}

TEST_F(TestSequencerNested, test_multi_nested_single_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, testNestedSingle);
    for (int i = 0; i < 3; i++) {
      theMsg->sendMsg<TestMsg, testSeqNestedSingleHan>(
        my_node, makeSharedMessage<TestMsg>()
      );
    }
    theTerm->attachGlobalTermAction([=]{
      testNestedSingle(-1);
    });
  }
}

TEST_F(TestSequencerNested, test_multi_deep_nested_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, testSeqDeepNested);
    for (int i = 0; i < 8; i++) {
      theMsg->sendMsg<TestMsg, testSeqDeepNestedHan>(
        my_node, makeSharedMessage<TestMsg>()
      );
    }
    theTerm->attachGlobalTermAction([=]{
      testSeqDeepNested(-1);
    });
  }
}


