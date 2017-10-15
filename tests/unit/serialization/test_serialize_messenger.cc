
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <tuple>
#include <type_traits>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

static constexpr int const val1 = 20;
static constexpr int const val2 = 21;
static constexpr int const val3 = 22;
static constexpr int const test_val = 129;

struct MyDataMsg : Message {
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

    for (auto&& elm: vec) {
      printf("elm=%d\n",elm);
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | vec;
    s | test;
  }
};

static void myDataMsgHan(MyDataMsg* msg) {
  printf("myDataMsgHandler: calling check\n");
  msg->check();
}

template <typename Tuple>
struct DataMsg : Message {
  using isByteCopyable = std::true_type;

  Tuple tup;

  DataMsg() = default;
  DataMsg(Tuple&& in_tup) : Message(), tup(std::forward<Tuple>(in_tup)) { }
};

struct TestSerialMessenger : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  template <typename Tuple>
  static void testHandler(DataMsg<Tuple>* msg) {
    auto const& tup = msg->tup;
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

  if (theContext->getNumNodes() > 1) {
    if (my_node == 0) {
      using TupleType = std::tuple<int, int, int>;

      auto msg = makeSharedMessage<DataMsg<TupleType>>(TupleType{val1,val2,val3});
      SerializedMessenger::sendSerialMsg<
        DataMsg<TupleType>, testHandler<TupleType>
      >(1, msg);
    }
  }
}

#if HAS_SERIALIZATION_LIBRARY
TEST_F(TestSerialMessenger, test_serial_messenger_2) {
  auto const& my_node = theContext->getNode();

  if (theContext->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeSharedMessage<MyDataMsg>();
      msg->init();
      SerializedMessenger::sendSerialMsg<MyDataMsg, myDataMsgHan>(1, msg);
    }
  }
}
#endif

}}} // end namespace vt::tests::unit
