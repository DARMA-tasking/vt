
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

  static void testNestedWaitFn(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->sequenced([]{
      theSeq->wait<TestMsg, testSeqNested>([](TestMsg* msg){
        EXPECT_EQ(seq_ordering_++, 1);
      });
    });
  }

  static void testNestedWaitFnMulti(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

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
  }
}

