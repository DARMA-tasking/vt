
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_collection_common.h"
#include "test_collection_construct_common.h"

#include <cstdint>
#include <tuple>
#include <string>

namespace vt { namespace tests { namespace unit {

namespace default_ {
struct ColMsg;
struct TestCol : Collection<TestCol,TestIndex> {
  using MsgType = ColMsg;
  using ParamType = std::tuple<>;
  TestCol()
    : Collection<TestCol, TestIndex>(),
      constructed_(true)
  { }
  bool isConstructed() const { return constructed_; }
private:
  bool constructed_ = false;
};
struct ColMsg : CollectionMessage<TestCol> { };
} /* end namespace default_ */

namespace index_ {
struct ColMsg;
struct TestCol : Collection<TestCol,TestIndex> {
  using MsgType = ColMsg;
  using ParamType = std::tuple<>;
  TestCol() = default;
  TestCol(TestIndex idx)
    : Collection<TestCol, TestIndex>(),
      constructed_(true)
  { }
  bool isConstructed() const { return constructed_; }
private:
  bool constructed_ = false;
};
struct ColMsg : CollectionMessage<TestCol> { };
} /* end namespace default_ */

/*
 *  These are now split into multiple files..this greatly improves compile-time
 *  for the tests
 */
// using CollectionTestTypes = testing::Types<
//   default_                       ::TestCol,
//   index_                         ::TestCol,
//   multi_param_idx_fst_           ::TestCol<int32_t>,
//   multi_param_idx_fst_           ::TestCol<int64_t>,
//   multi_param_idx_fst_           ::TestCol<std::string>,
//   multi_param_idx_fst_           ::TestCol<test_data::A>,
//   multi_param_idx_fst_           ::TestCol<test_data::B>,
//   multi_param_idx_fst_           ::TestCol<test_data::C>,
//   multi_param_idx_fst_           ::TestCol<int32_t,int32_t>,
//   multi_param_idx_fst_           ::TestCol<int64_t,int64_t>,
//   multi_param_idx_snd_           ::TestCol<int32_t>,
//   multi_param_idx_snd_           ::TestCol<int64_t>,
//   multi_param_idx_snd_           ::TestCol<std::string>,
//   multi_param_idx_snd_           ::TestCol<test_data::A>,
//   multi_param_idx_snd_           ::TestCol<test_data::B>,
//   multi_param_idx_snd_           ::TestCol<test_data::C>,
//   multi_param_idx_snd_           ::TestCol<int32_t,int32_t>,
//   multi_param_idx_snd_           ::TestCol<int64_t,int64_t>,
//   multi_param_no_idx_            ::TestCol<int32_t>,
//   multi_param_no_idx_            ::TestCol<int64_t>,
//   multi_param_no_idx_            ::TestCol<std::string>,
//   multi_param_no_idx_            ::TestCol<test_data::A>,
//   multi_param_no_idx_            ::TestCol<test_data::B>,
//   multi_param_no_idx_            ::TestCol<test_data::C>,
//   multi_param_no_idx_            ::TestCol<int32_t,int32_t>,
//   multi_param_no_idx_            ::TestCol<int64_t,int64_t>
// >;

using CollectionTestTypes = testing::Types<
  default_                       ::TestCol,
  index_                         ::TestCol
>;

INSTANTIATE_TYPED_TEST_CASE_P(
  test_construct_simple, TestConstruct, CollectionTestTypes
);

}}} // end namespace vt::tests::unit
