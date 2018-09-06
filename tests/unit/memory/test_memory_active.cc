
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

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

TYPED_TEST_CASE_P(TestMemoryActive);

TYPED_TEST_P(TestMemoryActive, test_memory_remote_send) {
  using MsgType = TypeParam;

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  bool const run_test = num_nodes > 1;
  NodeType const to_node = 1;

  std::vector<MsgType*> msgs;

  if (run_test && my_node == 0) {
    for (int i = 0; i < num_msg_sent; i++) {
      auto msg = makeSharedMessage<MsgType>();
      messageRef(msg);
      msgs.push_back(msg);
      theMsg()->sendMsg<MsgType,TestMemoryActive<MsgType>::test_handler>(
        to_node, msg
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
    for (auto msg : msgs) {
      // We expect 1 reference due to the messageRef above
      EXPECT_EQ(envelopeGetRef(msg->env), 1);
    }
  });
}

REGISTER_TYPED_TEST_CASE_P(TestMemoryActive, test_memory_remote_send);

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

INSTANTIATE_TYPED_TEST_CASE_P(test_mem_short,  TestMemoryActive, MsgShort);
INSTANTIATE_TYPED_TEST_CASE_P(test_mem_normal, TestMemoryActive, MsgNormal);
INSTANTIATE_TYPED_TEST_CASE_P(test_mem_serial, TestMemoryActive, MsgSerial);

}}} // end namespace vt::tests::unit
