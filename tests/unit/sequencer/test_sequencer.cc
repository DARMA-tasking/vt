
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

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
      printf("testSingleWaitFn seq_id=%d\n", seq_id);
    #endif

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, testSeqHan>([](TestMsg* msg){
      #if DEBUG_TEST_HARNESS_PRINT
        printf("testSingleWaitFn running wait\n");
      #endif

      EXPECT_EQ(seq_ordering_++, 1);
    });
  }

  static void testSingleTaggedWaitFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, testSeqTaggedHan>(single_tag, [](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1);
    });
  }

  static void testMultiWaitFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, testSeqMultiHan>([](TestMsg* msg){
      EXPECT_TRUE(seq_ordering_ == 1 or seq_ordering_ == 2);
      seq_ordering_++;
    });
    theSeq->wait<TestMsg, testSeqMultiHan>([](TestMsg* msg){
      EXPECT_TRUE(seq_ordering_ == 1 or seq_ordering_ == 2);
      seq_ordering_++;
    });
  }

  static void testMultiTaggedWaitFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->wait<TestMsg, testSeqMultiTaggedHan>(single_tag, [](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 1);
    });
    theSeq->wait<TestMsg, testSeqMultiTaggedHan>(single_tag_2, [](TestMsg* msg){
      EXPECT_EQ(seq_ordering_++, 2);
    });
  }
};

/*static*/ TagType TestSequencer::single_tag;
/*static*/ TagType TestSequencer::single_tag_2;

TEST_F(TestSequencer, test_single_wait) {
  auto const& my_node = theContext->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    printf("test_seq_handler: node=%d\n", my_node);
  #endif

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, testSingleWaitFn);

    theMsg->sendMsg<TestMsg, testSeqHan>(my_node, makeSharedMessage<TestMsg>());
  }
}

TEST_F(TestSequencer, test_single_wait_tagged) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, testSingleTaggedWaitFn);

    theMsg->sendMsg<TestMsg, testSeqTaggedHan>(
      my_node, makeSharedMessage<TestMsg>(), single_tag
    );
  }
}

TEST_F(TestSequencer, test_multi_wait) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, testMultiWaitFn);

    theMsg->sendMsg<TestMsg, testSeqMultiHan>(
      my_node, makeSharedMessage<TestMsg>()
    );
    theMsg->sendMsg<TestMsg, testSeqMultiHan>(
      my_node, makeSharedMessage<TestMsg>()
    );
  }
}

TEST_F(TestSequencer, test_multi_wait_tagged) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, testMultiTaggedWaitFn);

    theMsg->sendMsg<TestMsg, testSeqMultiTaggedHan>(
      my_node, makeSharedMessage<TestMsg>(), single_tag
    );
    theMsg->sendMsg<TestMsg, testSeqMultiTaggedHan>(
      my_node, makeSharedMessage<TestMsg>(), single_tag_2
    );
  }
}
