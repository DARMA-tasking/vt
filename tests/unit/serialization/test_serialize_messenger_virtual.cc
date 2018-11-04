
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

static constexpr int const val1 = 20;
static constexpr int const val2 = 21;
static constexpr int const val3 = 22;
static constexpr int const test_val = 129;

struct TestCtx : ::vt::vrt::VirtualContext {
  TestCtx() = default;
};

struct DataMsg : vt::vrt::VirtualMessage {
  int test = 0;
  std::vector<int> vec;

  void init() {
    vec = std::vector<int>{val1,val2};
    test = test_val;
  }

  void check() {
    EXPECT_EQ(test, test_val);
    EXPECT_EQ(vec[0], val1);
    EXPECT_EQ(vec[1], val2);

    // for (auto&& elm: vec) {
    //   fmt::print("elm={}\n",elm);
    // }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | vec;
    s | test;
  }
};

struct TestSerialMessengerVirtual : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static void testHandler(DataMsg* msg, TestCtx* ctx) {
    msg->check();
  }
};

#if HAS_SERIALIZATION_LIBRARY
TEST_F(TestSerialMessengerVirtual, test_serial_messenger_1) {
  auto const& my_node = theContext()->getNode();

  if (my_node == 0) {
    using TupleType = std::tuple<int, int>;

    auto proxy = theVirtualManager()->makeVirtual<TestCtx>();
    auto msg = makeSharedMessage<DataMsg>();
    msg->init();
    theVirtualManager()->sendSerialMsg<TestCtx, DataMsg, testHandler>(proxy, msg);
  }
}
#endif

}}} // end namespace vt::tests::unit
