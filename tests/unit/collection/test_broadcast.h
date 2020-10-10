/*
//@HEADER
// *****************************************************************************
//
//                               test_broadcast.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/transport.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit { namespace bcast {

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
  using MessageParentType = CollectionMessage<TestCol<Args...>>;
  using TupleType = std::tuple<Args...>;
  vt_msg_serialize_if_needed_by_parent_or_type1(TupleType);

  ColMsg() = default;
  explicit ColMsg(TupleType in_tup) : tup(in_tup) {}

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | tup;
  }

  TupleType tup;
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

TYPED_TEST_SUITE_P(TestBroadcast);

TYPED_TEST_P(TestBroadcast, test_broadcast_1) {
  using ColType = TypeParam;
  using MsgType = typename ColType::MsgType;
  using TestParamType = typename ColType::ParamType;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& col_size = 32;
    auto range = TestIndex(col_size);
    TestParamType args = ConstructTuple<TestParamType>::construct();
    auto proxy = theCollection()->construct<ColType>(range);

    proxy.template broadcast<
      MsgType,
      BroadcastHandlers<ColType>::handler
    >(args);

    auto msg = makeMessage<MsgType>(args);
    theCollection()->broadcastMsg<
      MsgType,BroadcastHandlers<ColType>::handler
    >(proxy, msg.get());
  }
}

}}}} // end namespace vt::tests::unit::bcast
