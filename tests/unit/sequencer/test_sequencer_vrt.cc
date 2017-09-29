
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
  static VrtType* test_vrt_ptr_a;
  static VrtType* test_vrt_ptr_b;

  SEQUENCE_REGISTER_VRT_HANDLER(VrtType, TestMsg, testSeqHan1);
  SEQUENCE_REGISTER_VRT_HANDLER(VrtType, TestMsg, testSeqHan2);
  SEQUENCE_REGISTER_VRT_HANDLER(VrtType, TestMsg, testSeqHan3);

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

  static void testSeqFn3a(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == FinalizeAtomicValue) {
      EXPECT_EQ(seq_ordering_++, 2);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theVrtSeq->wait<VrtType, TestMsg, testSeqHan3>([](TestMsg* msg, VrtType* vrt){
      printf("wait is triggered for a: msg=%p, vrt=%p\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr_a);
      EXPECT_EQ(seq_ordering_++, 1);
    });
  }

  static void testSeqFn3b(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == FinalizeAtomicValue) {
      EXPECT_EQ(seq_ordering_++, 2);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theVrtSeq->wait<VrtType, TestMsg, testSeqHan3>([](TestMsg* msg, VrtType* vrt){
      printf("wait is triggered for b: msg=%p, vrt=%p\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr_b);
      EXPECT_EQ(seq_ordering_++, 1);
    });
  }
};

/*static*/ TestSequencerVrt::TestVrt* TestSequencerVrt::test_vrt_ptr = nullptr;
/*static*/ TestSequencerVrt::TestVrt* TestSequencerVrt::test_vrt_ptr_a = nullptr;
/*static*/ TestSequencerVrt::TestVrt* TestSequencerVrt::test_vrt_ptr_b = nullptr;

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

TEST_F(TestSequencerVrt, test_seq_vc_distinct_inst_3) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    auto proxy_a = theVrtCManager->constructVrtContext<VrtType>(85);
    SeqType const& seq_id_a = theVrtSeq->createSeqVrtContext(proxy_a);
    auto vrt_ptr_a = theVrtCManager->getVrtContextByProxy(proxy_a);
    test_vrt_ptr_a = static_cast<VrtType*>(vrt_ptr_a);

    auto proxy_b = theVrtCManager->constructVrtContext<VrtType>(23);
    SeqType const& seq_id_b = theVrtSeq->createSeqVrtContext(proxy_b);
    auto vrt_ptr_b = theVrtCManager->getVrtContextByProxy(proxy_b);
    test_vrt_ptr_b = static_cast<VrtType*>(vrt_ptr_b);

    theVrtSeq->sequenced(seq_id_a, testSeqFn3a);
    theVrtSeq->sequenced(seq_id_b, testSeqFn3b);

    theVrtCManager->sendMsg<VrtType, TestMsg, testSeqHan3>(
      proxy_a, makeSharedMessage<TestMsg>()
    );
    theVrtCManager->sendMsg<VrtType, TestMsg, testSeqHan3>(
      proxy_b, makeSharedMessage<TestMsg>()
    );

    // @todo: fix this it is getting triggered early (a termination detector
    // bug?)
    theTerm->attachGlobalTermAction([=]{
      // testSeqFn3a(FinalizeAtomicValue);
      // testSeqFn3b(FinalizeAtomicValue);
    });
  }
}

}}} // end namespace vt::tests::unit
