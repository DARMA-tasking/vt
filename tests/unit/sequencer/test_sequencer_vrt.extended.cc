/*
//@HEADER
// *****************************************************************************
//
//                        test_sequencer_vrt.extended.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

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
      EXPECT_EQ(seq_ordering_++, 2U);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0U);

    theVirtualSeq()->wait<VirtualType, TestMsg, testSeqHan1>([](
      TestMsg* msg, VirtualType* vrt
    ){
      //fmt::print("wait is triggered: msg={}, vrt={}\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr);
      EXPECT_EQ(seq_ordering_++, 1U);
    });
  }

  static void testSeqFn2(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == FinalizeAtomicValue) {
      EXPECT_EQ(seq_ordering_++, 3U);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0U);

    theVirtualSeq()->wait<VirtualType, TestMsg, testSeqHan2>([](
      TestMsg* msg, VirtualType* vrt
    ){
      //fmt::print("wait is triggered: msg={}, vrt={}\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr);
      EXPECT_EQ(seq_ordering_++, 1U);
    });

    theVirtualSeq()->wait<VirtualType, TestMsg, testSeqHan2>([](
      TestMsg* msg, VirtualType* vrt
    ){
      //fmt::print("wait is triggered: msg={}, vrt={}\n", msg, vrt);
      EXPECT_EQ(vrt, test_vrt_ptr);
      EXPECT_EQ(seq_ordering_++, 2U);
    });
  }

  static void testSeqFn3a(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == FinalizeAtomicValue) {
      EXPECT_EQ(seq_ordering_++, 2U);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0U);

    theVirtualSeq()->wait<VirtualType, TestMsg, testSeqHan3>([](
      TestMsg* msg, VirtualType* vrt
    ){
      fmt::print(
        "wait is triggered for a: msg={}, vrt={}\n",
        print_ptr(msg), print_ptr(vrt)
      );
      EXPECT_EQ(vrt, test_vrt_ptr_a);
      EXPECT_EQ(seq_ordering_++, 1U);
    });
  }

  static void testSeqFn3b(SeqType const& seq_id) {
    static std::atomic<OrderType> seq_ordering_{};

    if (seq_id == FinalizeAtomicValue) {
      EXPECT_EQ(seq_ordering_++, 2U);
      return;
    }

    EXPECT_EQ(seq_ordering_++, 0U);

    theVirtualSeq()->wait<VirtualType, TestMsg, testSeqHan3>([](
      TestMsg* msg, VirtualType* vrt
    ){
      fmt::print(
        "wait is triggered for b: msg={}, vrt={}\n",
        print_ptr(msg), print_ptr(vrt)
      );
      EXPECT_EQ(vrt, test_vrt_ptr_b);
      EXPECT_EQ(seq_ordering_++, 1U);
    });
  }
};

/*static*/ TestSequencerVirtual::TestVirtual* TestSequencerVirtual::test_vrt_ptr = nullptr;
/*static*/ TestSequencerVirtual::TestVirtual* TestSequencerVirtual::test_vrt_ptr_a = nullptr;
/*static*/ TestSequencerVirtual::TestVirtual* TestSequencerVirtual::test_vrt_ptr_b = nullptr;

TEST_F(TestSequencerVirtual, test_seq_vc_1) {
  auto const& my_node = theContext()->getNode();

  runInEpochCollective([my_node]{
    if (my_node == 0) {
      auto proxy = theVirtualManager()->makeVirtual<VirtualType>(29);
      SeqType const& seq_id = theVirtualSeq()->createVirtualSeq(proxy);
      auto vrt_ptr = theVirtualManager()->getVirtualByProxy(proxy);

      test_vrt_ptr = static_cast<VirtualType*>(vrt_ptr);
      // fmt::print("vrt ptr={}\n", test_vrt_ptr);

      theVirtualSeq()->sequenced(seq_id, testSeqFn1);

      auto msg = makeMessage<TestMsg>();
      theVirtualManager()->sendMsg<VirtualType, TestMsg, testSeqHan1>(
        proxy, msg.get());
    }
  });

  if (my_node == 0) {
    testSeqFn1(FinalizeAtomicValue);
  }
}

TEST_F(TestSequencerVirtual, test_seq_vc_2) {
  auto const& my_node = theContext()->getNode();

  runInEpochCollective([my_node]{
    if (my_node == 0) {
      auto proxy = theVirtualManager()->makeVirtual<VirtualType>(85);
      SeqType const& seq_id = theVirtualSeq()->createVirtualSeq(proxy);
      auto vrt_ptr = theVirtualManager()->getVirtualByProxy(proxy);

      test_vrt_ptr = static_cast<VirtualType*>(vrt_ptr);

      theVirtualSeq()->sequenced(seq_id, testSeqFn2);

      for (int i = 0; i < 2; i++) {
        auto msg = makeMessage<TestMsg>();
        theVirtualManager()->sendMsg<VirtualType, TestMsg, testSeqHan2>(
          proxy, msg.get());
      }
    }
  });

  if (my_node == 0) {
    testSeqFn2(FinalizeAtomicValue);
  }
}

TEST_F(TestSequencerVirtual, test_seq_vc_distinct_inst_3) {
  auto const& my_node = theContext()->getNode();

  runInEpochCollective([my_node]{
    if (my_node == 0) {
      auto proxy_a = theVirtualManager()->makeVirtual<VirtualType>(85);
      SeqType const& seq_id_a = theVirtualSeq()->createVirtualSeq(proxy_a);
      auto vrt_ptr_a = theVirtualManager()->getVirtualByProxy(proxy_a);
      test_vrt_ptr_a = static_cast<VirtualType*>(vrt_ptr_a);

      auto proxy_b = theVirtualManager()->makeVirtual<VirtualType>(23);
      SeqType const& seq_id_b = theVirtualSeq()->createVirtualSeq(proxy_b);
      auto vrt_ptr_b = theVirtualManager()->getVirtualByProxy(proxy_b);
      test_vrt_ptr_b = static_cast<VirtualType*>(vrt_ptr_b);

      theVirtualSeq()->sequenced(seq_id_a, testSeqFn3a);
      theVirtualSeq()->sequenced(seq_id_b, testSeqFn3b);

      auto msg1 = makeMessage<TestMsg>();
      theVirtualManager()->sendMsg<VirtualType, TestMsg, testSeqHan3>(
        proxy_a, msg1.get());
      auto msg2 = makeMessage<TestMsg>();
      theVirtualManager()->sendMsg<VirtualType, TestMsg, testSeqHan3>(
        proxy_b, msg2.get());
    }
  });

  if (my_node == 0) {
    testSeqFn3a(FinalizeAtomicValue);
    testSeqFn3b(FinalizeAtomicValue);
  }
}
}}} // end namespace vt::tests::unit
