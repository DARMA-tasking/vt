
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

static constexpr int const val1 = 20;
static constexpr int const val2 = 21;
static constexpr int const val3 = 22;

struct TestCtx : ::vt::vrt::VirtualContext {
  TestCtx() = default;
};

template <typename Tuple>
struct DataMsg : ShortMessage {
  Tuple* tup = nullptr;
  DataMsg(Tuple* in_tup) : ShortMessage(), tup(in_tup) { }
};

struct TestSerialMessengerVirtual : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  template <typename Tuple, typename VirtualContextT>
  static void testHandler(DataMsg<Tuple>* msg, VirtualContextT* ctx) {
    EXPECT_EQ(std::get<0>(*msg->tup), val1);
    EXPECT_EQ(std::get<1>(*msg->tup)[0], val2);
    EXPECT_EQ(std::get<1>(*msg->tup)[1], val3);
  }
};

TEST_F(TestSerialMessengerVirtual, test_serial_messenger_1) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    using TupleType = std::tuple<int, std::vector<int>>;

    SerializedMessenger::sendSerialVirtualMsg<
      TestCtx, DataMsg<TupleType>, testHandler<TupleType, TestCtx>
    >(1, TupleType{val1,std::vector<int>{val2,val3}});
  }
}

}}} // end namespace vt::tests::unit
