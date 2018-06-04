
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "transport.h"

#include <cstdint>
#include <tuple>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;
using namespace vt::vrt;
using namespace vt::vrt::collection;

namespace send_col_ {
template <typename... Args> struct ColMsg;
template <typename... Args>
struct TestCol : Collection<TestCol<Args...>,TestIndex> {
  using MsgType = ColMsg<Args...>;
  using ParamType = std::tuple<Args...>;
  TestCol() = default;
  void testMethod(Args... args) {
    #if PRINT_CONSTRUCTOR_VALUES
      ConstructTuple<ParamType>::print(std::make_tuple(args...));
    #endif
    ConstructTuple<ParamType>::isCorrect(std::make_tuple(args...));
  }
};
template <typename... Args>
struct ColMsg : CollectionMessage<TestCol<Args...>> {
  using TupleType = std::tuple<Args...>;
  ColMsg() = default;
  explicit ColMsg(TupleType in_tup) : tup(in_tup) {}
  TupleType tup;
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | tup;
  }
};
} /* end namespace send_col_ */

template <
  typename CollectionT,
  typename MessageT = typename CollectionT::MsgType,
  typename TupleT   = typename MessageT::TupleType
>
struct SendHandlers {
  static void handler(MessageT* msg, CollectionT* col) {
    return execute(col,msg->tup);
  }
  template <typename... Args>
  static void execute(CollectionT* col, std::tuple<Args...> args) {
    return unpack(col,args,std::index_sequence_for<Args...>{});
  }
  template <std::size_t ...I>
  static void unpack(
    CollectionT* col, TupleT args, std::index_sequence<I...>
  ) {
    return col->testMethod(std::get<I>(args)...);
  }
};

template <typename CollectionT>
struct TestCollectionSend : TestParallelHarness {};

TYPED_TEST_CASE_P(TestCollectionSend);

TYPED_TEST_P(TestCollectionSend, test_collection_send_1) {
  using ColType = TypeParam;
  using MsgType = typename ColType::MsgType;
  using ParamType = typename ColType::ParamType;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& col_size = 32;
    auto range = TestIndex(col_size);
    ParamType args = ConstructTuple<ParamType>::construct();
    auto proxy = theCollection()->construct<ColType>(range);
    for (int i = 0; i < col_size; i++) {
      auto msg = makeSharedMessage<MsgType>(args);
      //proxy[i].template send<MsgType,SendHandlers<ColType>::handler>(msg);
      if (i % 2 == 0) {
        proxy[i].template send<MsgType,SendHandlers<ColType>::handler>(msg);
      } else {
        theCollection()->sendMsg<MsgType,SendHandlers<ColType>::handler>(
          proxy[i], msg, nullptr
        );
      }
    }
  }
}

REGISTER_TYPED_TEST_CASE_P(TestCollectionSend, test_collection_send_1);

using CollectionTestTypes = testing::Types<
  send_col_            ::TestCol<int32_t>,
  send_col_            ::TestCol<int64_t>,
  send_col_            ::TestCol<std::string>,
  send_col_            ::TestCol<test_data::A>,
  send_col_            ::TestCol<test_data::B>,
  send_col_            ::TestCol<test_data::C>,
  send_col_            ::TestCol<int32_t,int32_t>,
  send_col_            ::TestCol<int64_t,int64_t>
>;

INSTANTIATE_TYPED_TEST_CASE_P(
  test_collection_send, TestCollectionSend, CollectionTestTypes
);

}}} // end namespace vt::tests::unit
