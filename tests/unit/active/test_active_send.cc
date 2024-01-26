/*
//@HEADER
// *****************************************************************************
//
//                             test_active_send.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include "vt/messaging/active.h"
#include "test_parallel_harness.h"
#include "data_message.h"
#include "test_helpers.h"

namespace vt { namespace tests { namespace unit { namespace send {

using namespace vt;
using namespace vt::tests::unit;

struct PutTestMessage : ::vt::PayloadMessage {
  PutTestMessage() = default;
};

struct DataMsg : ::vt::Message {
  using MessageParentType = ::vt::Message; // base message
  vt_msg_serialize_required();             // mark serialization mode

  DataMsg() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s); // ensure parent is serialized consistently

    s | data;
  }

public:
  std::vector<uint32_t> data;
};

struct TestActiveSend : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static inline NodeT from_node = {};
  static inline NodeT to_node = {};

  static inline int handler_count = {};
  static inline int num_msg_sent = {};

  virtual void SetUp() {
    TestParallelHarness::SetUp();

    handler_count = 0;
    num_msg_sent = 16;

    from_node = NodeT{0};
    to_node = NodeT{1};
  }

  static void test_handler_small_put(PutTestMessage* msg) {
    auto ptr = static_cast<int*>(msg->getPut());
    auto size = msg->getPutSize();
    #if DEBUG_TEST_HARNESS_PRINT
      auto const& this_node = theContext()->getNode();
      fmt::print(
        "{}: test_handler_small_put: size={}, ptr={}\n", this_node, size, print_ptr(ptr)
      );
    #endif
    EXPECT_EQ(2 * sizeof(int), size);
    for (int i = 0; i < 2; i++) {
      EXPECT_EQ(ptr[i], i);
    }
  }

  static void test_handler_large_put(PutTestMessage* msg) {
    auto ptr = static_cast<int*>(msg->getPut());
    auto size = msg->getPutSize();
    #if DEBUG_TEST_HARNESS_PRINT
      auto const& this_node = theContext()->getNode();
      fmt::print(
        "{}: test_handler_large_put: size={}, ptr={}\n", this_node, size, print_ptr(ptr)
      );
    #endif
    EXPECT_EQ(10 * sizeof(int), size);
    for (int i = 0; i < 10; i++) {
      EXPECT_EQ(ptr[i], i);
    }
  }

  static void test_handler(TestMsg* msg) {
    EXPECT_TRUE(envelopeIsLocked(msg->env)) << "Should be locked on recv";

    auto const& this_node = theContext()->getNode();

    #if DEBUG_TEST_HARNESS_PRINT
      fmt::print("{}: test_handler: cnt={}\n", this_node, handler_count);
    #endif

    handler_count++;

    EXPECT_EQ(this_node, to_node);
  }

  static void msgSerialA(DataMsg*) { handler_count++; }
};

TEST_F(TestActiveSend, test_type_safe_active_fn_send) {
  SET_MIN_NUM_NODES_CONSTRAINT(2);
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_send: node={}\n", my_node);
  #endif

  vt::runInEpochCollective([&]{
    if (my_node == from_node) {
      for (int i = 0; i < num_msg_sent; i++) {
        #if DEBUG_TEST_HARNESS_PRINT
          fmt::print("{}: sendMsg: i={}\n", my_node, i);
        #endif

        auto msg = makeMessage<TestMsg>();
        auto msg_hold = promoteMsg(msg.get());

        theMsg()->sendMsg<test_handler>(to_node, msg);
        EXPECT_TRUE(envelopeIsLocked(msg_hold->env)) << "Should be locked on send";
      }
    }
  });

  if (my_node == to_node) {
    EXPECT_EQ(handler_count, num_msg_sent);
  }

  // Spin here so test_vec does not go out of scope before the send completes
  vt::theSched()->runSchedulerWhile([]{ return !rt->isTerminated(); });
}

TEST_F(TestActiveSend, test_type_safe_active_fn_send_small_put) {
  SET_MIN_NUM_NODES_CONSTRAINT(2);
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_send_small_put: node={}\n", my_node);
  #endif

  std::vector<int> test_vec{0,1};

  if (my_node == from_node) {
    for (int i = 0; i < num_msg_sent; i++) {
      auto msg = makeMessage<PutTestMessage>();
      msg->setPut(test_vec.data(), sizeof(int)*test_vec.size());
      #if DEBUG_TEST_HARNESS_PRINT
        fmt::print("{}: sendMsg: (put) i={}\n", my_node, i);
      #endif
      theMsg()->sendMsg<test_handler_small_put>(to_node, msg);
    }
  }

  // Spin here so test_vec does not go out of scope before the send completes
  vt::theSched()->runSchedulerWhile([]{ return !rt->isTerminated(); });
}

TEST_F(TestActiveSend, test_type_safe_active_fn_send_large_put) {
  SET_MIN_NUM_NODES_CONSTRAINT(2);
  auto const& my_node = theContext()->getNode();

  #if DEBUG_TEST_HARNESS_PRINT
    fmt::print("test_type_safe_active_fn_send_large_put: node={}\n", my_node);
  #endif

  std::vector<int> test_vec_2{0,1,2,3,4,5,6,7,8,9};

  if (my_node == from_node) {
    for (int i = 0; i < num_msg_sent; i++) {
      auto msg = makeMessage<PutTestMessage>();
      msg->setPut(test_vec_2.data(), sizeof(int)*test_vec_2.size());
      #if DEBUG_TEST_HARNESS_PRINT
        fmt::print("{}: sendMsg: (put) i={}\n", my_node, i);
      #endif
      theMsg()->sendMsg<test_handler_large_put>(to_node, msg);
    }
  }

  // Spin here so test_vec does not go out of scope before the send completes
  vt::theSched()->runSchedulerWhile([]{ return !rt->isTerminated(); });
}

TEST_F(TestActiveSend, test_active_message_serialization) {
  vt::runInEpochCollective([] {
    auto const this_node = theContext()->getNode();

    for (auto i = 0; i < num_msg_sent; ++i) {
      auto msg = vt::makeMessage<DataMsg>();
      msg->data.resize(serialization::serialized_msg_eager_size);

      theMsg()->sendMsg<msgSerialA>(this_node, msg);
    }
  });

  EXPECT_EQ(handler_count, num_msg_sent);
}

void testPropertiesHandler(int a, double b) {
  // do nothing
}

TEST_F(TestActiveSend, test_active_message_properties) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();
  NodeT const next_node = (this_node + 1) % num_nodes;

  if (num_nodes > 1) {
    auto ps = theMsg()->send<testPropertiesHandler>(
      vt::NodeT{next_node}, MsgProps().asTerminationMsg(), 10, 20.0
    );
    EXPECT_TRUE(messaging::envelopeIsTerm(ps.getMsg()->env));
  }
}

}}}} // end namespace vt::tests::unit::send
