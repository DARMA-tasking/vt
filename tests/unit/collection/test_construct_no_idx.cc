
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_collection_common.h"
#include "test_collection_construct_common.h"

#include <cstdint>
#include <tuple>
#include <string>

namespace vt { namespace tests { namespace unit {

namespace multi_param_no_idx_ {
template <typename... Args> struct ColMsg;
template <typename... Args>
struct TestCol : Collection<TestCol<Args...>,TestIndex>, BaseCol {
  using MsgType = ColMsg<Args...>;
  using ParamType = std::tuple<Args...>;
  TestCol() = default;
  TestCol(Args... args)
    : Collection<TestCol, TestIndex>(),
      BaseCol(true)
  {
    #if PRINT_CONSTRUCTOR_VALUES
      ConstructTuple<ParamType>::print(std::make_tuple(args...));
    #endif
    ConstructTuple<ParamType>::isCorrect(std::make_tuple(args...));
  }
};
template <typename... Args>
struct ColMsg : CollectionMessage<TestCol<Args...>> {};
} /* end namespace multi_param_no_idx_ */

using CollectionTestTypes = testing::Types<
  multi_param_no_idx_            ::TestCol<int32_t>,
  multi_param_no_idx_            ::TestCol<int64_t>,
  multi_param_no_idx_            ::TestCol<std::string>,
  multi_param_no_idx_            ::TestCol<test_data::A>,
  multi_param_no_idx_            ::TestCol<test_data::B>,
  multi_param_no_idx_            ::TestCol<test_data::C>,
  multi_param_no_idx_            ::TestCol<int32_t,int32_t>,
  multi_param_no_idx_            ::TestCol<int64_t,int64_t>
>;

INSTANTIATE_TYPED_TEST_CASE_P(
  test_construct_no_idx, TestConstruct, CollectionTestTypes
);

INSTANTIATE_TYPED_TEST_CASE_P(
  test_construct_no_idx_dist, TestConstructDist, CollectionTestTypes
);

}}} // end namespace vt::tests::unit
