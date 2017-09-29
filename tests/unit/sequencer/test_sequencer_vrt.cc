
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::vrt;
using namespace vt::tests::unit;

static constexpr SeqType const FinalizeAtomicValue = -1;

template <NumBytesType num_bytes>
using TestVCMsg = TestStaticBytesMsg<vt::vrt::VrtContextMessage, num_bytes>;

struct TestSequencerVrt : TestParallelHarness {
  using TestMsg = TestVCMsg<4>;
  using OrderType = uint32_t;

  struct TestVrt : VrtContext {
    int test_data = 10;

    TestVrt(int const& test_data_in)
      : test_data(test_data_in)
    { }
  };

  using VrtType = TestVrt;

  static VrtType* test_vrt_ptr;

  SEQUENCE_REGISTER_VRT_HANDLER(
    TestSequencerVrt::VrtType, TestSequencerVrt::TestMsg, testSeqHan1
  );
  SEQUENCE_REGISTER_VRT_HANDLER(
    TestSequencerVrt::VrtType, TestSequencerVrt::TestMsg, testSeqHan2
  );

  static void testSeqFn1(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == FinalizeAtomicValue) {
      EXPECT_EQ(seq_ordering_++, 2);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theVrtSeq->wait<VrtType, TestMsg, testSeqHan1>([](TestMsg* msg, VrtType* vrt){
      //printf("wait is triggered: msg=%p, vrt=%p\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr);
      EXPECT_EQ(seq_ordering_++, 1);
    });
  }

  static void testSeqFn2(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == FinalizeAtomicValue) {
      EXPECT_EQ(seq_ordering_++, 3);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theVrtSeq->wait<VrtType, TestMsg, testSeqHan2>([](TestMsg* msg, VrtType* vrt){
      //printf("wait is triggered: msg=%p, vrt=%p\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr);
      EXPECT_EQ(seq_ordering_++, 1);
    });

    theVrtSeq->wait<VrtType, TestMsg, testSeqHan2>([](TestMsg* msg, VrtType* vrt){
      //printf("wait is triggered: msg=%p, vrt=%p\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr);
      EXPECT_EQ(seq_ordering_++, 2);
    });
  }
};

/*static*/ TestSequencerVrt::TestVrt* TestSequencerVrt::test_vrt_ptr = nullptr;

TEST_F(TestSequencerVrt, test_seq_vc_1) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    auto proxy = theVrtCManager->constructVrtContext<VrtType>(29);
    SeqType const& seq_id = theVrtSeq->createSeqVrtContext(proxy);
    auto vrt_ptr = theVrtCManager->getVrtContextByProxy(proxy);

    test_vrt_ptr = static_cast<VrtType*>(vrt_ptr);
    //printf("vrt ptr=%p\n", test_vrt_ptr);

    theVrtSeq->sequenced(seq_id, testSeqFn1);

    theVrtCManager->sendMsg<VrtType, TestMsg, testSeqHan1>(
      proxy, makeSharedMessage<TestMsg>()
    );

    theTerm->attachGlobalTermAction([=]{
      testSeqFn1(FinalizeAtomicValue);
    });
  }
}

TEST_F(TestSequencerVrt, test_seq_vc_2) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    auto proxy = theVrtCManager->constructVrtContext<VrtType>(85);
    SeqType const& seq_id = theVrtSeq->createSeqVrtContext(proxy);
    auto vrt_ptr = theVrtCManager->getVrtContextByProxy(proxy);

    test_vrt_ptr = static_cast<VrtType*>(vrt_ptr);

    theVrtSeq->sequenced(seq_id, testSeqFn2);

    for (int i = 0; i < 2; i++) {
      theVrtCManager->sendMsg<VrtType, TestMsg, testSeqHan2>(
        proxy, makeSharedMessage<TestMsg>()
      );
    }

    theTerm->attachGlobalTermAction([=]{
      testSeqFn2(FinalizeAtomicValue);
    });
  }
}

}}} // end namespace vt::tests::unit
