
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

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
      printf("testSeqForFn seq_id=%d\n", seq_id);
    #endif

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 11);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->for_loop(0, end_range, 1, [](vt::seq::ForIndex i) {
      theSeq->wait_closure<TestMsg, testSeqForHan>(no_tag, [=](TestMsg* msg){
        #if DEBUG_TEST_HARNESS_PRINT
          printf("testSeqForFn running wait\n");
        #endif

        EXPECT_EQ(seq_ordering_++, i+1);
      });
    });
  }
};

TEST_F(TestSequencerFor, test_for) {
  auto const& my_node = theContext->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    printf("test_seq_handler: node=%d\n", my_node);
  #endif

  if (my_node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, testSeqForFn);

    for (int i = 0; i < end_range; i++) {
      theMsg->sendMsg<TestMsg, testSeqForHan>(
        my_node, makeSharedMessage<TestMsg>()
      );
    }

    theTerm->attachGlobalTermAction([=]{
      testSeqForFn(-1);
    });
  }
}

}}} // end namespace vt::tests::unit
