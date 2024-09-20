/*
//@HEADER
// *****************************************************************************
//
//                         test_index_types.extended.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/vrt/collection/manager.h"

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
  using MessageParentType = CollectionMessage<TestCol<IndexT>>;
  vt_msg_serialize_if_needed_by_parent();

  ColMsg() = default;
  explicit ColMsg(int in_data) : data_(in_data) {}

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | data_;
  }

  int data_;
};

template <typename IndexT>
void TestCol<IndexT>::handler([[maybe_unused]] ColMsg<IndexT>* msg) {}

} /* end namespace test_index_types_ */


template <typename CollectionT>
struct TestCollectionIndexTypes : TestParallelHarness {};

TYPED_TEST_SUITE_P(TestCollectionIndexTypes);

TYPED_TEST_P(TestCollectionIndexTypes, test_collection_index_1) {
  using IndexType     = TypeParam;
  using ColType       = test_index_types_::TestCol<IndexType>;
  using MsgType       = test_index_types_::ColMsg<IndexType>;
  using BaseIndexType = typename IndexType::DenseIndexType;

  auto const& this_node = theContext()->getNode();

  if (this_node == 0) {
    auto const& col_size = 32;
    auto range = IndexType(static_cast<BaseIndexType>(col_size));
    auto proxy = theCollection()->construct<ColType>(
      range, "test_collection_index_1"
    );
    for (BaseIndexType i = 0; i < static_cast<BaseIndexType>(col_size); i++) {
      auto msg = makeMessage<MsgType>(34);
      if (i % 2 == 0) {
        proxy[i].template sendMsg<MsgType,&ColType::handler>(msg.get());
      } else {
        theCollection()->sendMsg<MsgType,&ColType::handler>(
          proxy[i], msg.get()
        );
      }
    }
  }
}

REGISTER_TYPED_TEST_SUITE_P(TestCollectionIndexTypes, test_collection_index_1);

using CollectionTestTypes = testing::Types<
  ::vt::Index1D,
  ::vt::IdxType1D<int32_t>,
  ::vt::IdxType1D<int16_t>,
  ::vt::IdxType1D<int8_t>,
  ::vt::IdxType1D<int64_t>,
  ::vt::IdxType1D<std::size_t>
>;

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_index_types, TestCollectionIndexTypes, CollectionTestTypes, DEFAULT_NAME_GEN
);

}}} // end namespace vt::tests::unit
