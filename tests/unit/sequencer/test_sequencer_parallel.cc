
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

using namespace vt;
using namespace vt::tests::unit;

#if DEBUG_TEST_HARNESS_PRINT
#define DEBUG_PRINT_SEQ_NESTED(ORDER, CUR, LABEL)                       \
  do {                                                                  \
    auto seq_id = theSeq->getCurrentSeq();                              \
    printf(                                                             \
      "seqParFnA (" LABEL "): seq_id=%d, ordering=%d -- cur=%d --\n",   \
      seq_id, (ORDER).load(), (CUR)                                     \
    );                                                                  \
  } while (false);
#else
#define DEBUG_PRINT_SEQ_NESTED(ORDER, CUR, INIT)
#endif

struct TestSequencerParallel : TestParallelHarness {
  using TestMsg = TestStaticBytesNormalMsg<4>;
  using OrderType = uint32_t;

  static TagType single_tag;
  static TagType single_tag_2;

  virtual void SetUp() {
    TestParallelHarness::SetUp();

    TestSequencerParallel::single_tag = 29;
    TestSequencerParallel::single_tag_2 = 31;
  }

  SEQUENCE_REGISTER_HANDLER(TestSequencerParallel::TestMsg, seqParHanA);

  static void seqParFnA(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == -1) {
      EXPECT_EQ(seq_ordering_++, 3);
      return;
    }

    DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 0, "INIT");

    EXPECT_EQ(seq_ordering_++, 0);

    theSeq->parallel(seq_id, []{
      theSeq->wait<TestMsg, seqParHanA>([](TestMsg* msg){
        DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 1, "PAR-1");
        auto const val = seq_ordering_.fetch_add(1);
        EXPECT_TRUE(val == 1 or val == 2);
      });
    },[]{
      theSeq->wait<TestMsg, seqParHanA>([](TestMsg* msg){
        DEBUG_PRINT_SEQ_NESTED(seq_ordering_, 2, "PAR-2");
        auto const val = seq_ordering_.fetch_add(1);
        EXPECT_TRUE(val == 1 or val == 2);
      });
    });
  }
};

/*static*/ TagType TestSequencerParallel::single_tag;
/*static*/ TagType TestSequencerParallel::single_tag_2;

TEST_F(TestSequencerParallel, test_parallel_1) {
  auto const& node = theContext->getNode();

  if (node == 0) {
    SeqType const& seq_id = theSeq->nextSeq();
    theSeq->sequenced(seq_id, seqParFnA);
    for (int i = 0; i < 2; i++) {
      theMsg->sendMsg<TestMsg, seqParHanA>(node, makeSharedMessage<TestMsg>());
    }
    theTerm->attachGlobalTermAction([=]{ seqParFnA(-1); });
  }
}
