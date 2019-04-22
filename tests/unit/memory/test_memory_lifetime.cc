/*
//@HEADER
// ************************************************************************
//
//                   test_active_send_lifetime.cc
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

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TrackMsg : ::vt::Message { };

struct SerialTrackMsg : ::vt::Message {
  SerialTrackMsg() { ++alloc_count; }
  SerialTrackMsg(SerialTrackMsg const& other) { ++alloc_count; }
  SerialTrackMsg(SerialTrackMsg&& other) { ++alloc_count; }

  template <typename Serializer>
  void serialize(Serializer&) { }

  ~SerialTrackMsg() { --alloc_count; }

  static int32_t alloc_count;
};

int32_t SerialTrackMsg::alloc_count = 0;

struct TestMemoryLifetime : TestParallelHarness {
  using NormalTestMsg = TrackMsg;
  using SerialTestMsg = SerialTrackMsg;

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

TEST_F(TestMemoryLifetime, test_active_send_serial_lifetime_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    auto const next_node = this_node + 1 < num_nodes ? this_node + 1 : 0;
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeSharedMessage<SerialTestMsg>();
      theMsg()->sendMsgAuto<SerialTestMsg, serialHan>(next_node, msg);
    }
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeMessage<SerialTestMsg>();
      theMsg()->sendMsgAuto<SerialTestMsg, serialHan>(next_node, msg.get());
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(SerialTrackMsg::alloc_count, 0);
      EXPECT_EQ(local_count, num_msgs_sent*2);
    });
  }
}

TEST_F(TestMemoryLifetime, test_active_bcast_serial_lifetime_1) {
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeSharedMessage<SerialTestMsg>();
      theMsg()->broadcastMsgAuto<SerialTestMsg, serialHan>(msg);
    }
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeMessage<SerialTestMsg>();
      theMsg()->broadcastMsgAuto<SerialTestMsg, serialHan>(msg.get());
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(SerialTrackMsg::alloc_count, 0);
      EXPECT_EQ(local_count, num_msgs_sent*(num_nodes-1)*2);
    });
  }
}

TEST_F(TestMemoryLifetime, test_active_send_normal_lifetime_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    auto const next_node = this_node + 1 < num_nodes ? this_node + 1 : 0;
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeSharedMessage<NormalTestMsg>();
      envelopeRef(msg->env);
      theMsg()->sendMsg<NormalTestMsg, normalHan>(next_node, msg);

      theTerm()->addAction([msg]{
        // Call event cleanup all pending MPI requests to clear
        theEvent()->cleanup();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
        envelopeDeref(msg->env);
      });
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(local_count, num_msgs_sent);
    });
  }
}

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
        theEvent()->cleanup();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
      });
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(local_count, num_msgs_sent);
    });
  }
}

TEST_F(TestMemoryLifetime, test_active_bcast_normal_lifetime_1) {
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeSharedMessage<NormalTestMsg>();
      envelopeRef(msg->env);
      theMsg()->broadcastMsg<NormalTestMsg, normalHan>(msg);

      theTerm()->addAction([msg]{
        // Call event cleanup all pending MPI requests to clear
        theEvent()->cleanup();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
        envelopeDeref(msg->env);
      });
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(local_count, num_msgs_sent*(num_nodes-1));
    });
  }
}

TEST_F(TestMemoryLifetime, test_active_bcast_normal_lifetime_2) {
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes > 1) {
    for (int i = 0; i < num_msgs_sent; i++) {
      auto msg = makeMessage<NormalTestMsg>();
      theMsg()->broadcastMsg<NormalTestMsg, normalHan>(msg.get());

      theTerm()->addAction([msg]{
        // Call event cleanup all pending MPI requests to clear
        theEvent()->cleanup();
        EXPECT_EQ(envelopeGetRef(msg->env), 1);
      });
    }

    theTerm()->addAction([=]{
      EXPECT_EQ(local_count, num_msgs_sent*(num_nodes-1));
    });
  }
}


}}} // end namespace vt::tests::unit
