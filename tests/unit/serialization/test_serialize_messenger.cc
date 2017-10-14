
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

template <typename Tuple>
struct DataMsg : ShortMessage {
  Tuple* tup = nullptr;
  DataMsg(Tuple* in_tup) : ShortMessage(), tup(in_tup) { }
};

struct TestSerialMessenger : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  template <typename Tuple>
  static void testHandler(DataMsg<Tuple>* msg) {
    auto tup = *msg->tup;
    auto const& v1 = std::get<0>(tup);
    auto const& v2 = std::get<1>(tup);
    auto const& v3 = std::get<2>(tup);
    EXPECT_EQ(v1, val1);
    EXPECT_EQ(v2, val2);
    EXPECT_EQ(v3, val3);
  }
};

TEST_F(TestSerialMessenger, test_serial_messenger_1) {
  auto const& my_node = theContext->getNode();

  if (my_node == 0) {
    using TupleType = std::tuple<int, int, int>;

    SerializedMessenger::sendSerialMsg<
      DataMsg<TupleType>, testHandler<TupleType>
    >(1, TupleType{val1,val2,val3});
  }
}

}}} // end namespace vt::tests::unit
