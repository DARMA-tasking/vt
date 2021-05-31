/*
//@HEADER
// *****************************************************************************
//
//                                 ping_pong.cc
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
#include "common/test_harness.h"
#include <vt/collective/collective_ops.h>
#include <vt/messaging/active.h>

#include <cassert>
#include <cstdint>

#include <fmt/core.h>

#define DEBUG_PING_PONG 0
#define REUSE_MESSAGE_PING_PONG 0

using namespace vt;
using namespace vt::tests::perf::common;

static constexpr int64_t const min_bytes = 1;
static constexpr int64_t const max_bytes = 16384;

static int64_t num_pings = 1024;

static constexpr NodeType const ping_node = 0;
static constexpr NodeType const pong_node = 1;

template <int64_t num_bytes>
struct PingMsg : ShortMessage {
  int64_t count = 0;
  std::array<char, num_bytes> payload;

  PingMsg() : ShortMessage() { }
  PingMsg(int64_t const in_count) : ShortMessage(), count(in_count) { }
};

template <int64_t num_bytes>
struct FinishedPingMsg : ShortMessage {
  int64_t prev_bytes = 0;

  FinishedPingMsg(int64_t const in_prev_bytes)
    : ShortMessage(), prev_bytes(in_prev_bytes)
  { }
};

struct MyTest : PerfTestHarness {
  static void printTiming(int64_t const& num_bytes) {
    auto const& name = fmt::format("{}", num_bytes);
    AddResult({name, timers_[name].Stop()});
  }

  template <int64_t num_bytes>
  static void finishedPing(FinishedPingMsg<num_bytes>* msg) {
    printTiming(num_bytes);

    if (num_bytes != max_bytes) {
      timers_[fmt::format("{}", num_bytes * 2)].Start();

      auto pmsg = makeMessage<PingMsg<num_bytes * 2>>();
      theMsg()->sendMsg<PingMsg<num_bytes * 2>, pingPong<num_bytes * 2>>(
        pong_node, pmsg);
    }
  }

  template <int64_t num_bytes>
  static void pingPong(PingMsg<num_bytes>* in_msg) {
    auto const& cnt = in_msg->count;

    if (cnt >= num_pings) {
      auto msg = makeMessage<FinishedPingMsg<num_bytes>>(num_bytes);
      theMsg()->sendMsg<FinishedPingMsg<num_bytes>, finishedPing<num_bytes>>(
        0, msg);
    } else {
      NodeType const next =
        theContext()->getNode() == ping_node ? pong_node : ping_node;

      auto m = makeMessage<PingMsg<num_bytes>>(cnt + 1);
      theMsg()->sendMsg<PingMsg<num_bytes>, pingPong<num_bytes>>(next, m);
    }
  }
};

template <>
void MyTest::finishedPing<max_bytes>(FinishedPingMsg<max_bytes>* msg) {
  MyTest::printTiming(max_bytes);
}

VT_PERF_TEST(MyTest, test_ping_pong) {
  if (my_node_ == 0) {
    auto m = makeMessage<PingMsg<min_bytes>>();
    theMsg()->sendMsg<PingMsg<min_bytes>, pingPong<min_bytes>>(pong_node, m);
  }
}

VT_PERF_TEST_MAIN()
