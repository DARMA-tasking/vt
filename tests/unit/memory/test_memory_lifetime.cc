/*
//@HEADER
// *****************************************************************************
//
//                           test_memory_lifetime.cc
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
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TrackMsg : ::vt::Message { };

struct SerialTrackMsg : ::vt::Message {
  // Must be serialized because byte-copyable are supposed
  // to be trivially destructible. However, the check has
  // been historically broken so..
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_required();

  SerialTrackMsg() { ++alloc_count; }
  SerialTrackMsg(SerialTrackMsg const& other) { ++alloc_count; }
  SerialTrackMsg(SerialTrackMsg&& other) { ++alloc_count; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    MessageParentType::serialize(s);
  }

  ~SerialTrackMsg() { --alloc_count; }

  static int32_t alloc_count;
};

template <typename MsgT>
struct CallbackMsg : vt::Message {
  CallbackMsg() = default;
  CallbackMsg(Callback<MsgT> in_cb) : cb_(in_cb) { }

  Callback<MsgT> cb_;
};

int32_t SerialTrackMsg::alloc_count = 0;

using NormalTestMsg = TrackMsg;
using SerialTestMsg = SerialTrackMsg;

struct TestMemoryLifetime : TestParallelHarness {
  virtual void SetUp() {
    TestParallelHarness::SetUp();
    SerialTestMsg::alloc_count = 0;
    local_count = 0;
  }

  static void serialHan(SerialTestMsg* msg) {
    local_count++;
  }

  static void normalHan(NormalTestMsg* msg) {
    local_count++;
    //fmt::print("{}: normalHan num={}\n", theContext()->getNode(), local_count);
  }

  static int32_t local_count;
};

int32_t TestMemoryLifetime::local_count = 0;

static constexpr int32_t const num_msgs_sent = 32;

////////////////////////////////////////////////////////////////////////////////
// Active message send, serialized
////////////////////////////////////////////////////////////////////////////////
TEST_F(TestMemoryLifetime, test_active_send_serial_lifetime_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    runInEpochCollective([&]{
      auto const next_node = this_node + 1 < num_nodes ? this_node + 1 : 0;
      for (int i = 0; i < num_msgs_sent; i++) {
        auto msg = makeMessage<SerialTestMsg>();
        theMsg()->sendMsg<SerialTestMsg, serialHan>(next_node, msg.get());
      }
      for (int i = 0; i < num_msgs_sent; i++) {
        auto msg = makeMessage<SerialTestMsg>();
        theMsg()->sendMsg<SerialTestMsg, serialHan>(next_node, msg.get());
      }
    });

    EXPECT_EQ(SerialTrackMsg::alloc_count, 0);
    EXPECT_EQ(local_count, num_msgs_sent*2);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Active message broadcast, serialized
////////////////////////////////////////////////////////////////////////////////
TEST_F(TestMemoryLifetime, test_active_bcast_serial_lifetime_1) {
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    runInEpochCollective([&]{
      for (int i = 0; i < num_msgs_sent; i++) {
        auto msg = makeMessage<SerialTestMsg>();
        theMsg()->broadcastMsg<SerialTestMsg, serialHan>(msg.get());
      }
      for (int i = 0; i < num_msgs_sent; i++) {
        auto msg = makeMessage<SerialTestMsg>();
        theMsg()->broadcastMsg<SerialTestMsg, serialHan>(msg.get());
      }
    });

    EXPECT_EQ(SerialTrackMsg::alloc_count, 0);
    EXPECT_EQ(local_count, num_msgs_sent*(num_nodes-1)*2);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Active message send, non-serialized, MsgT*
////////////////////////////////////////////////////////////////////////////////
TEST_F(TestMemoryLifetime, test_active_send_normal_lifetime_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    auto const next_node = this_node + 1 < num_nodes ? this_node + 1 : 0;
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeMessage<NormalTestMsg>();
      theMsg()->sendMsg<NormalTestMsg, normalHan>(next_node, msg.get());

      theTerm()->addAction([msg]{
        // Call event cleanup all pending MPI requests to clear
        theEvent()->finalize();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
      });
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(local_count, num_msgs_sent);
    });
  }
}

////////////////////////////////////////////////////////////////////////////////
// Active message send, non-serialized, MsgSharedPtr
////////////////////////////////////////////////////////////////////////////////
TEST_F(TestMemoryLifetime, test_active_send_normal_lifetime_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    auto const next_node = this_node + 1 < num_nodes ? this_node + 1 : 0;
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeMessage<NormalTestMsg>();
      theMsg()->sendMsg<NormalTestMsg, normalHan>(next_node, msg.get());

      theTerm()->addAction([msg]{
        // Call event cleanup all pending MPI requests to clear
        theEvent()->finalize();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
      });
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(local_count, num_msgs_sent);
    });
  }
}

////////////////////////////////////////////////////////////////////////////////
// Active message broadcast, non-serialized, MsgT*
////////////////////////////////////////////////////////////////////////////////
TEST_F(TestMemoryLifetime, test_active_bcast_normal_lifetime_1) {
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeMessage<NormalTestMsg>();
      theMsg()->broadcastMsg<NormalTestMsg, normalHan>(msg.get());

      theTerm()->addAction([msg]{
        // Call event cleanup all pending MPI requests to clear
        theEvent()->finalize();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
      });
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(local_count, num_msgs_sent*(num_nodes-1));
    });
  }
}

////////////////////////////////////////////////////////////////////////////////
// Active message broadcast, non-serialized, MsgSharedPtr
////////////////////////////////////////////////////////////////////////////////
TEST_F(TestMemoryLifetime, test_active_bcast_normal_lifetime_2) {
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeMessage<NormalTestMsg>();
      theMsg()->broadcastMsg<NormalTestMsg, normalHan>(msg.get());

      theTerm()->addAction([msg]{
        // Call event cleanup all pending MPI requests to clear
        theEvent()->finalize();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
      });
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(local_count, num_msgs_sent*(num_nodes-1));
    });
  }
}


////////////////////////////////////////////////////////////////////////////////
// Callback func, non-serialized
////////////////////////////////////////////////////////////////////////////////
static void callbackHan(CallbackMsg<NormalTestMsg>* msg) {
  auto send_msg = makeMessage<NormalTestMsg>();
  msg->cb_.send(send_msg.get());

  theTerm()->addAction([send_msg]{
    // Call event cleanup all pending MPI requests to clear
    theEvent()->finalize();
    EXPECT_EQ(envelopeGetRef(send_msg->env), 1);
  });
}

TEST_F(TestMemoryLifetime, test_active_send_callback_lifetime_1) {
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    auto cb = theCB()->makeFunc<NormalTestMsg>([](NormalTestMsg* msg){ });

    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeMessage<CallbackMsg<NormalTestMsg>>(cb);
      theMsg()->broadcastMsg<CallbackMsg<NormalTestMsg>, callbackHan>(msg.get());

      theTerm()->addAction([msg]{
        // Call event cleanup all pending MPI requests to clear
        theEvent()->finalize();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
      });
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Callback func, serialized
////////////////////////////////////////////////////////////////////////////////
static void callbackHan(CallbackMsg<SerialTestMsg>* msg) {
  auto send_msg = makeMessage<SerialTestMsg>();
  msg->cb_.send(send_msg.get());

  theTerm()->addAction([send_msg]{
    // Call event cleanup all pending MPI requests to clear
    theEvent()->finalize();
    EXPECT_EQ(envelopeGetRef(send_msg->env), 1);
  });
}

TEST_F(TestMemoryLifetime, test_active_serial_callback_lifetime_1) {
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    auto cb = theCB()->makeFunc<SerialTestMsg>([](SerialTestMsg* msg){ });

    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeMessage<CallbackMsg<SerialTestMsg>>(cb);
      theMsg()->broadcastMsg<CallbackMsg<SerialTestMsg>, callbackHan>(msg.get());

      theTerm()->addAction([msg]{
        // Call event cleanup all pending MPI requests to clear
        theEvent()->finalize();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
      });
    }
  }
}


}}} // end namespace vt::tests::unit
