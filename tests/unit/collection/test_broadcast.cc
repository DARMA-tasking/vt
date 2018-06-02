
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "transport.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;
using namespace vt::vrt;
using namespace vt::vrt::collection;

namespace bcast_col_ {
template <typename... Args> struct ColMsg;
template <typename... Args>
struct TestCol : Collection<TestCol<Args...>,TestIndex> {
  using MsgType = ColMsg<Args...>;
  using ParamType = std::tuple<Args...>;
  TestCol() = default;
  void testMethod(Args... args) {
    #if PRINT_CONSTRUCTOR_VALUES || 1
      ConstructTuple<ParamType>::print(std::make_tuple(args...));
    #endif
    ConstructTuple<ParamType>::isCorrect(std::make_tuple(args...));
  }
};
template <typename... Args>
struct ColMsg : CollectionMessage<TestCol<Args...>> {
  explicit ColMsg(std::tuple<Args...> in_tup) : tup(in_tup) {}
  std::tuple<Args...> tup;
  template <typename SerializerT>
  void serialize(SerializerT& s) { s | tup; }
};
} /* end namespace bcast_col_ */

struct BroadcastHandlers {
  template <
    typename CollectionT,
    typename MessageT = typename CollectionT::MsgType
  >
  static void handler(MessageT* msg, CollectionT* col) {
    return executeTup<CollectionT,decltype(msg->tup)>(col,msg->tup);
  }
  template <typename CollectionT, typename Tuple>
  static void executeTup(CollectionT* col,Tuple args) {
    return execute<CollectionT,Tuple>(col,args);
  }
  template <typename CollectionT, typename Tuple, typename... Args>
  static void execute(CollectionT* col, std::tuple<Args...> args) {
    return unpack(col,args,std::index_sequence_for<Args...>{});
  }
  template <typename CollectionT, typename Tuple, std::size_t ...I>
  static void unpack(
    CollectionT* col, Tuple args, std::index_sequence<I...>
  ) {
    return col->testMethod(std::get<I>(args)...);
  }
};

template <typename CollectionT>
struct TestBroadcast : TestParallelHarness {};

TYPED_TEST_CASE_P(TestBroadcast);

TYPED_TEST_P(TestBroadcast, test_broadcast_1) {
  using ColType = TypeParam;
  using MsgType = typename ColType::MsgType;
  using ParamType = typename ColType::ParamType;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& col_size = 32;
    auto range = TestIndex(col_size);
    ParamType args = ConstructTuple<ParamType>::construct();
    auto proxy = theCollection()->construct<ColType>(range);
    auto msg = makeSharedMessage<MsgType>(args);
    proxy.template broadcast<
      MsgType,
      BroadcastHandlers::handler<ColType,MsgType>
    >(msg);
  }
}

REGISTER_TYPED_TEST_CASE_P(TestBroadcast, test_broadcast_1);

using CollectionTestTypes = testing::Types<
  bcast_col_            ::TestCol<std::string>,
  bcast_col_            ::TestCol<int64_t>,
  bcast_col_            ::TestCol<std::string>,
  bcast_col_            ::TestCol<test_data::A>,
  bcast_col_            ::TestCol<test_data::B>,
  bcast_col_            ::TestCol<test_data::C>,
  bcast_col_            ::TestCol<int32_t,int32_t>,
  bcast_col_            ::TestCol<int64_t,int64_t>
>;

INSTANTIATE_TYPED_TEST_CASE_P(
  test_bcast, TestBroadcast, CollectionTestTypes
);

}}} // end namespace vt::tests::unit
