/*
//@HEADER
// ************************************************************************
//
//                          test_broadcast.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/transport.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

namespace bcast_col_ {
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
} /* end namespace bcast_col_ */

template <
  typename CollectionT,
  typename MessageT = typename CollectionT::MsgType,
  typename TupleT   = typename MessageT::TupleType
>
struct BroadcastHandlers {
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
      BroadcastHandlers<ColType>::handler
    >(msg);

    auto msg2 = makeSharedMessage<MsgType>(args);
    theCollection()->broadcastMsg<
      MsgType,BroadcastHandlers<ColType>::handler
    >(proxy, msg2);
  }
}

REGISTER_TYPED_TEST_CASE_P(TestBroadcast, test_broadcast_1);

using CollectionTestTypes = testing::Types<
  bcast_col_            ::TestCol<int32_t>,
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
