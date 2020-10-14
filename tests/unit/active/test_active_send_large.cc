/*
//@HEADER
// *****************************************************************************
//
//                          test_active_send_large.cc
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

#include <vt/transport.h>
#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"

namespace vt { namespace tests { namespace unit { namespace large {

struct SerializedTag {};
struct NonSerializedTag {};

using RecvMsg = vt::Message;

struct CallbackMsg : vt::Message {
  vt::Callback<RecvMsg> cb_;
};

template <NumBytesType nbytes, typename T, typename enabled_=void>
struct LargeMsg;

template <NumBytesType nbytes, typename T>
struct LargeMsg<
  nbytes,
  T,
  typename std::enable_if_t<std::is_same<T, SerializedTag>::value>
> : TestStaticSerialBytesMsg<CallbackMsg, nbytes> { };

template <NumBytesType nbytes, typename T>
struct LargeMsg<
  nbytes,
  T,
  typename std::enable_if_t<std::is_same<T, NonSerializedTag>::value>
> : TestStaticBytesMsg<CallbackMsg, nbytes> { };

template <typename T>
struct TestActiveSendLarge : TestParallelHarness {
  using TagType = typename std::tuple_element<1,T>::type;

  template <typename MsgT>
  static void myHandler(MsgT* m) {
    auto msg = makeMessage<RecvMsg>();
    m->cb_.send(msg.get());
  }
};

TYPED_TEST_SUITE_P(TestActiveSendLarge);

TYPED_TEST_P(TestActiveSendLarge, test_large_bytes_msg) {
  using IntegralType = typename std::tuple_element<0,TypeParam>::type;
  using TagType = typename std::tuple_element<1,TypeParam>::type;

  static constexpr NumBytesType const nbytes = 1ll << IntegralType::value;

  using ThisType = TestActiveSendLarge<TypeParam>;
  using LargeMsgType = LargeMsg<nbytes, TagType>;

  NodeType const this_node = theContext()->getNode();
  NodeType const num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  int counter = 0;
  auto e = pipe::LifetimeEnum::Once;
  auto cb = theCB()->makeFunc<RecvMsg>(e, [&counter](RecvMsg*){ counter++; });

  vt::runInEpochCollective([&]{
    NodeType next_node = (this_node + 1) % num_nodes;
    auto msg = makeMessage<LargeMsgType>();
    msg->cb_ = cb;
    theMsg()->sendMsg<LargeMsgType, ThisType::template myHandler<LargeMsgType>>(
      next_node, msg
    );
  });

  EXPECT_EQ(counter, 1);
}

REGISTER_TYPED_TEST_SUITE_P(TestActiveSendLarge, test_large_bytes_msg);

using NonSerTestTypes = testing::Types<
  std::tuple<std::integral_constant<NumBytesType, 30>, NonSerializedTag>,
  std::tuple<std::integral_constant<NumBytesType, 31>, NonSerializedTag>,
  std::tuple<std::integral_constant<NumBytesType, 34>, NonSerializedTag>
>;

using SerTestTypes = testing::Types<
  std::tuple<std::integral_constant<NumBytesType, 30>, SerializedTag>,
  std::tuple<std::integral_constant<NumBytesType, 31>, SerializedTag>,
  std::tuple<std::integral_constant<NumBytesType, 34>, SerializedTag>
>;

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_large_bytes_serialized, TestActiveSendLarge, NonSerTestTypes,
  DEFAULT_NAME_GEN
);

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_large_bytes_nonserialized, TestActiveSendLarge, SerTestTypes,
  DEFAULT_NAME_GEN
);

}}}} // end namespace vt::tests::unit::large
