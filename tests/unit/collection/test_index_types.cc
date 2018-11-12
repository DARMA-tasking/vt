
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

#include <cstdint>
#include <tuple>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

namespace test_index_types_ {
template <typename IndexT> struct ColMsg;
template <typename IndexT>
struct TestCol : Collection<TestCol<IndexT>,IndexT> {
  using MsgType = ColMsg<IndexT>;
  TestCol() = default;
  void handler(ColMsg<IndexT>* msg);
};

template <typename IndexT>
struct ColMsg : CollectionMessage<TestCol<IndexT>> {
  ColMsg() = default;
  explicit ColMsg(int in_data) : data_(in_data) {}
  int data_;
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | data_;
  }
};

template <typename IndexT>
void TestCol<IndexT>::handler(ColMsg<IndexT>* msg) {}

} /* end namespace test_index_types_ */


template <typename CollectionT>
struct TestCsollectionIndexTypes : TestParallelHarness {};

TYPED_TEST_CASE_P(TestCsollectionIndexTypes);

TYPED_TEST_P(TestCsollectionIndexTypes, test_collection_index_1) {
  using IndexType     = TypeParam;
  using ColType       = test_index_types_::TestCol<IndexType>;
  using MsgType       = test_index_types_::ColMsg<IndexType>;
  using BaseIndexType = typename IndexType::DenseIndexType;

  auto const& this_node = theContext()->getNode();

  if (this_node == 0) {
    auto const& col_size = 32;
    auto range = IndexType(static_cast<BaseIndexType>(col_size));
    auto proxy = theCollection()->construct<ColType>(range);
    for (BaseIndexType i = 0; i < col_size; i++) {
      auto msg = makeSharedMessage<MsgType>(34);
      if (i % 2 == 0) {
        proxy[i].template send<MsgType,&ColType::handler>(msg);
      } else {
        theCollection()->sendMsg<MsgType,&ColType::handler>(
          proxy[i], msg, nullptr
        );
      }
    }
  }
}

REGISTER_TYPED_TEST_CASE_P(TestCsollectionIndexTypes, test_collection_index_1);

using CollectionTestTypes = testing::Types<
  ::vt::Index1D,
  ::vt::IdxType1D<int32_t>,
  ::vt::IdxType1D<int16_t>,
  ::vt::IdxType1D<int8_t>,
  ::vt::IdxType1D<int64_t>,
  ::vt::IdxType1D<std::size_t>
>;

INSTANTIATE_TYPED_TEST_CASE_P(
  test_collection_index, TestCsollectionIndexTypes, CollectionTestTypes
);

}}} // end namespace vt::tests::unit
