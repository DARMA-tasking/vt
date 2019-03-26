/*
//@HEADER
// ************************************************************************
//
//                          test_serialize_messenger.cc
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

#include <tuple>
#include <type_traits>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

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
      fmt::print("elm={}\n",elm);
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | vec;
    s | test;
  }
};

static void myDataMsgHan(MyDataMsg* msg) {
  fmt::print("myDataMsgHandler: calling check\n");
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

  template <typename TupleT>
  static void testBcastHandler(DataMsg<TupleT>* msg) {
    auto const& node = theContext()->getNode();
    //fmt::print("{}:testBcastHandler\n",node);
    return testHandler<TupleT>(msg);
  }
};

TEST_F(TestSerialMessenger, test_serial_messenger_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      using TupleType = std::tuple<int, int, int>;

      auto msg = makeSharedMessage<DataMsg<TupleType>>(TupleType{val1,val2,val3});
      SerializedMessenger::sendSerialMsg<
        DataMsg<TupleType>, testHandler<TupleType>
      >(1, msg);
    }
  }
}

TEST_F(TestSerialMessenger, test_serial_messenger_bcast_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      using TupleType = std::tuple<int, int, int>;

      auto msg = makeSharedMessage<DataMsg<TupleType>>(TupleType{val1,val2,val3});
      SerializedMessenger::broadcastSerialMsg<
        DataMsg<TupleType>, testBcastHandler<TupleType>
      >(msg);
    }
  }
}


#if HAS_SERIALIZATION_LIBRARY
TEST_F(TestSerialMessenger, test_serial_messenger_2) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeSharedMessage<MyDataMsg>();
      msg->init();
      SerializedMessenger::sendSerialMsg<MyDataMsg, myDataMsgHan>(1, msg);
    }
  }
}
#endif

}}} // end namespace vt::tests::unit
