
#if !defined INCLUDED_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H
#define INCLUDED_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/transport.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <tuple>
#include <string>

#define PRINT_CONSTRUCTOR_VALUES 0

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct ConstructHandlers {
  template <
    typename CollectionT,
    typename MessageT = typename CollectionT::MsgType
  >
  static void handler(MessageT* msg, CollectionT* col) {
    // fmt::print(
    //   "{}: constructed TestCol: idx.x()={}\n",
    //   theContext()->getNode(), col->getInex().x(), print_ptr(col)
    // );
    EXPECT_TRUE(col->isConstructed());
  }
};

template <typename CollectionT>
struct TestConstruct : TestParallelHarness {};

template <typename ColT, typename Tuple>
struct ConstructParams {
  using IndexType = typename ColT::IndexType;
  using ProxyType = CollectionIndexProxy<ColT,IndexType>;
  static ProxyType constructTup(IndexType idx,Tuple args) {
    return construct(idx,args);
  }
  template <typename... Args>
  static ProxyType construct(IndexType idx, std::tuple<Args...> args) {
    return unpack(idx,args,std::index_sequence_for<Args...>{});
  }
  template <std::size_t ...I>
  static ProxyType unpack(
    IndexType idx, Tuple args, std::index_sequence<I...>
  ) {
    return theCollection()->construct<ColT>(idx,std::get<I>(args)...);
  }
};

struct BaseCol {
  BaseCol() = default;
  explicit BaseCol(bool const in_constructed)
    : constructed_(in_constructed)
  {}
  bool isConstructed() const { return constructed_; }
protected:
  bool constructed_ = false;
};

TYPED_TEST_CASE_P(TestConstruct);

TYPED_TEST_P(TestConstruct, test_construct_1) {
  using ColType = TypeParam;
  using MsgType = typename ColType::MsgType;
  using ParamType = typename ColType::ParamType;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& col_size = 32;
    auto range = TestIndex(col_size);
    ParamType args = ConstructTuple<ParamType>::construct();
    auto proxy = ConstructParams<ColType,ParamType>::constructTup(range,args);
    auto msg = makeSharedMessage<MsgType>();
    proxy.template broadcast<
      MsgType,
      ConstructHandlers::handler<ColType,MsgType>
    >(msg);
  }
}

REGISTER_TYPED_TEST_CASE_P(TestConstruct, test_construct_1);

}}} // end namespace vt::tests::unit

#endif /*INCLUDED_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H*/
