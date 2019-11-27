/*
//@HEADER
// *****************************************************************************
//
//                            test_memory_active.cc
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

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

#include <vector>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

template <template <NumBytesType> class MsgT>
struct TestMemoryActiveMsg {
  using TestMsgA = MsgT<4>;
  using TestMsgB = MsgT<64>;
  using TestMsgC = MsgT<2048>;
};

template <typename MsgT>
struct TestMemoryActive : TestParallelHarness {
  static void test_handler(MsgT* msg) { }
};

static constexpr int32_t const num_msg_sent = 5;

TYPED_TEST_SUITE_P(TestMemoryActive);

TYPED_TEST_P(TestMemoryActive, test_memory_remote_send) {
  using MsgType = TypeParam;

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  bool const run_test = num_nodes > 1;
  NodeType const to_node = 1;

  std::vector<MsgSharedPtr<MsgType>> msgs;

  if (run_test && my_node == 0) {
    for (int i = 0; i < num_msg_sent; i++) {
      auto msg = makeMessage<MsgType>();
      msgs.push_back(msg);
      theMsg()->sendMsgAuto<MsgType,TestMemoryActive<MsgType>::test_handler>(
        to_node, msg.get()
      );
    }
  }

  theTerm()->addAction([=]{
    /*
     *  Explicitly call event cleanup so any pending MPI requests get tested and
     *  thus the memory gets freed (or dereferenced)---the corresponding
     *  messages we are checking for a correct reference count go to 1
     */
    theEvent()->cleanup();
    for (auto&& msg : msgs) {
      // We expect 1 reference due to the messageRef above
      EXPECT_EQ(envelopeGetRef(msg->env), 1);
    }
  });
}

TYPED_TEST_P(TestMemoryActive, test_memory_remote_broadcast) {
  using MsgType = TypeParam;

  auto const& num_nodes = theContext()->getNumNodes();

  std::vector<MsgSharedPtr<MsgType>> msgs;

  if (num_nodes > 1) {
    for (int i = 0; i < num_msg_sent; i++) {
      auto msg = makeMessage<MsgType>();
      msgs.push_back(msg);
      theMsg()->broadcastMsgAuto<MsgType,TestMemoryActive<MsgType>::test_handler>(
        msg.get()
      );
    }
  }

  theTerm()->addAction([=]{
    /*
     *  Explicitly call event cleanup so any pending MPI requests get tested and
     *  thus the memory gets freed (or dereferenced)---the corresponding
     *  messages we are checking for a correct reference count go to 1
     */
    theEvent()->cleanup();
    for (auto&& msg : msgs) {
      // We expect 1 reference due to the messageRef above
      EXPECT_EQ(envelopeGetRef(msg->env), 1);
    }
  });
}

REGISTER_TYPED_TEST_SUITE_P(
  TestMemoryActive, test_memory_remote_send, test_memory_remote_broadcast
);

using MsgShort = testing::Types<
  TestMemoryActiveMsg<TestStaticBytesShortMsg>::TestMsgA,
  TestMemoryActiveMsg<TestStaticBytesShortMsg>::TestMsgB,
  TestMemoryActiveMsg<TestStaticBytesShortMsg>::TestMsgC
>;

using MsgNormal = testing::Types<
  TestMemoryActiveMsg<TestStaticBytesNormalMsg>::TestMsgA,
  TestMemoryActiveMsg<TestStaticBytesNormalMsg>::TestMsgB,
  TestMemoryActiveMsg<TestStaticBytesNormalMsg>::TestMsgC
>;

using MsgSerial = testing::Types<
  TestMemoryActiveMsg<TestStaticBytesSerialMsg>::TestMsgA,
  TestMemoryActiveMsg<TestStaticBytesSerialMsg>::TestMsgB,
  TestMemoryActiveMsg<TestStaticBytesSerialMsg>::TestMsgC
>;

INSTANTIATE_TYPED_TEST_SUITE_P(test_mem_short,  TestMemoryActive, MsgShort);
INSTANTIATE_TYPED_TEST_SUITE_P(test_mem_normal, TestMemoryActive, MsgNormal);
INSTANTIATE_TYPED_TEST_SUITE_P(test_mem_serial, TestMemoryActive, MsgSerial);

}}} // end namespace vt::tests::unit
