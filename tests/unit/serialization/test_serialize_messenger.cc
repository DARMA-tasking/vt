/*
//@HEADER
// *****************************************************************************
//
//                         test_serialize_messenger.cc
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
static constexpr int const test_val = 129;

struct MyDataMsg : Message {
  using MessageParentType = Message;
  vt_msg_serialize_required();

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
    MessageParentType::serialize(s);
    s | vec;
    s | test;
  }
};

static void myDataMsgHan(MyDataMsg* msg) {
  fmt::print("myDataMsgHandler: calling check\n");
  msg->check();
}

struct TestSerialMessenger : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;
};

TEST_F(TestSerialMessenger, test_serial_messenger_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeMessage<MyDataMsg>();
      msg->init();
      auto han = auto_registry::makeAutoHandler<MyDataMsg,myDataMsgHan>(nullptr);
      SerializedMessenger::sendSerialMsg<MyDataMsg>(1, msg.get(), han);
    }
  }
}

TEST_F(TestSerialMessenger, test_serial_messenger_bcast_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeMessage<MyDataMsg>();
      msg->init();
      auto han = auto_registry::makeAutoHandler<MyDataMsg,myDataMsgHan>(nullptr);
      SerializedMessenger::broadcastSerialMsg<MyDataMsg>(msg.get(), han);
    }
  }
}

}}} // end namespace vt::tests::unit
