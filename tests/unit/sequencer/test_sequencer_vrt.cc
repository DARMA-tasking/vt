
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
using TestVCMsg = TestStaticBytesMsg<vt::vrt::VirtualMessage, num_bytes>;

struct TestSequencerVirtual : TestParallelHarness {
  using TestMsg = TestVCMsg<4>;
  using OrderType = uint32_t;

  struct TestVirtual : VirtualContext {
    int test_data = 10;

    TestVirtual(int const& test_data_in)
      : test_data(test_data_in)
    { }
  };

  using VirtualType = TestVirtual;

  static VirtualType* test_vrt_ptr;
  static VirtualType* test_vrt_ptr_a;
  static VirtualType* test_vrt_ptr_b;

  SEQUENCE_REGISTER_VRT_HANDLER(VirtualType, TestMsg, testSeqHan1);
  SEQUENCE_REGISTER_VRT_HANDLER(VirtualType, TestMsg, testSeqHan2);
  SEQUENCE_REGISTER_VRT_HANDLER(VirtualType, TestMsg, testSeqHan3);

  static void testSeqFn1(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == FinalizeAtomicValue) {
      EXPECT_EQ(seq_ordering_++, 2);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0);

    theVirtualSeq->wait<VirtualType, TestMsg, testSeqHan1>([](
      TestMsg* msg, VirtualType* vrt
    ){
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

    theVirtualSeq->wait<VirtualType, TestMsg, testSeqHan2>([](
      TestMsg* msg, VirtualType* vrt
    ){
      //printf("wait is triggered: msg=%p, vrt=%p\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr);
      EXPECT_EQ(seq_ordering_++, 1);
    });

    theVirtualSeq->wait<VirtualType, TestMsg, testSeqHan2>([](
      TestMsg* msg, VirtualType* vrt
    ){
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

    theVirtualSeq->wait<VirtualType, TestMsg, testSeqHan3>([](
      TestMsg* msg, VirtualType* vrt
    ){
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

    theVirtualSeq->wait<VirtualType, TestMsg, testSeqHan3>([](
      TestMsg* msg, VirtualType* vrt
    ){
      printf("wait is triggered for b: msg=%p, vrt=%p\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr_b);
      EXPECT_EQ(seq_ordering_++, 1);
    });
  }
};

/*static*/ TestSequencerVirtual::TestVirtual* TestSequencerVirtual::test_vrt_ptr = nullptr;
/*static*/ TestSequencerVirtual::TestVirtual* TestSequencerVirtual::test_vrt_ptr_a = nullptr;
/*static*/ TestSequencerVirtual::TestVirtual* TestSequencerVirtual::test_vrt_ptr_b = nullptr;

TEST_F(TestSequencerVirtual, test_seq_vc_1) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    auto proxy = theVirtualManager->makeVirtual<VirtualType>(29);
    SeqType const& seq_id = theVirtualSeq->createVirtualSeq(proxy);
    auto vrt_ptr = theVirtualManager->getVirtualByProxy(proxy);

    test_vrt_ptr = static_cast<VirtualType*>(vrt_ptr);
    //printf("vrt ptr=%p\n", test_vrt_ptr);

    theVirtualSeq->sequenced(seq_id, testSeqFn1);

    theVirtualManager->sendMsg<VirtualType, TestMsg, testSeqHan1>(
      proxy, makeSharedMessage<TestMsg>()
    );

    theTerm->attachGlobalTermAction([=]{
      testSeqFn1(FinalizeAtomicValue);
    });
  }
}

TEST_F(TestSequencerVirtual, test_seq_vc_2) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    auto proxy = theVirtualManager->makeVirtual<VirtualType>(85);
    SeqType const& seq_id = theVirtualSeq->createVirtualSeq(proxy);
    auto vrt_ptr = theVirtualManager->getVirtualByProxy(proxy);

    test_vrt_ptr = static_cast<VirtualType*>(vrt_ptr);

    theVirtualSeq->sequenced(seq_id, testSeqFn2);

    for (int i = 0; i < 2; i++) {
      theVirtualManager->sendMsg<VirtualType, TestMsg, testSeqHan2>(
        proxy, makeSharedMessage<TestMsg>()
      );
    }

    theTerm->attachGlobalTermAction([=]{
      testSeqFn2(FinalizeAtomicValue);
    });
  }
}

TEST_F(TestSequencerVirtual, test_seq_vc_distinct_inst_3) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    auto proxy_a = theVirtualManager->makeVirtual<VirtualType>(85);
    SeqType const& seq_id_a = theVirtualSeq->createVirtualSeq(proxy_a);
    auto vrt_ptr_a = theVirtualManager->getVirtualByProxy(proxy_a);
    test_vrt_ptr_a = static_cast<VirtualType*>(vrt_ptr_a);

    auto proxy_b = theVirtualManager->makeVirtual<VirtualType>(23);
    SeqType const& seq_id_b = theVirtualSeq->createVirtualSeq(proxy_b);
    auto vrt_ptr_b = theVirtualManager->getVirtualByProxy(proxy_b);
    test_vrt_ptr_b = static_cast<VirtualType*>(vrt_ptr_b);

    theVirtualSeq->sequenced(seq_id_a, testSeqFn3a);
    theVirtualSeq->sequenced(seq_id_b, testSeqFn3b);

    theVirtualManager->sendMsg<VirtualType, TestMsg, testSeqHan3>(
      proxy_a, makeSharedMessage<TestMsg>()
    );
    theVirtualManager->sendMsg<VirtualType, TestMsg, testSeqHan3>(
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
