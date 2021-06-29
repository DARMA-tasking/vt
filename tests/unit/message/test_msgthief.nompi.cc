/*
//@HEADER
// *****************************************************************************
//
//                            test_msgthief.nompi.cc
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

// TODO: change to pure unit after #990
#include "test_parallel_harness.h"
#include "vt/messaging/message/shared_message.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestMsg : ::vt::Message { };

struct TestMsgThief : TestParallelHarness {
  int callWithMsgThief(messaging::MsgPtrThief<TestMsg> msg) {
    // What's the ref-count here, before ownership is relinquished?
    auto& stolen = msg.msg_;
    return envelopeGetRef(stolen->env);
  }
};

TEST_F(TestMsgThief, test_msgthief_deprecated_rawptr) {
  auto msg = makeMessage<TestMsg>();
  EXPECT_EQ(msg.ownsMessage(), true);
  EXPECT_EQ(envelopeGetRef(msg->env), 1);

  // The obsolete API is expected to be used as a migration shim.
  // It is forced to take a NEW ref.
  auto* rawptr = msg.get();
  int internalRef = callWithMsgThief(rawptr); // T*
  EXPECT_EQ(internalRef, 2) << "message accepted (increases ref)";

  EXPECT_EQ(msg.ownsMessage(), true) << "original MsgPtr remains valid";
  EXPECT_EQ(envelopeGetRef(msg->env), 1);
}

TEST_F(TestMsgThief, test_msgthief_implicit_stealing) {
  auto msg = makeMessage<TestMsg>();
  EXPECT_EQ(msg.ownsMessage(), true);
  EXPECT_EQ(envelopeGetRef(msg->env), 1);

  int internalRef = callWithMsgThief(msg); // MsgPtr<T>&
  EXPECT_EQ(internalRef, 1) << "message stolen (same ref count)";

  EXPECT_EQ(msg.ownsMessage(), false) << "MsgPtr invalidated";
}

TEST_F(TestMsgThief, test_msgthief_explicit_stdmove) {
  auto msg = makeMessage<TestMsg>();
  EXPECT_EQ(msg.ownsMessage(), true);
  EXPECT_EQ(envelopeGetRef(msg->env), 1);

  int internalRef = callWithMsgThief(std::move(msg)); // MsgPtr<T>&&
  EXPECT_EQ(internalRef, 1) << "message stolen (same ref count)";

  EXPECT_EQ(msg.ownsMessage(), false) << "MsgPtr invalidated";
}

}}} // end namespace vt::tests::unit
