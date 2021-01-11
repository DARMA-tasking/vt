/*
//@HEADER
// *****************************************************************************
//
//                              comm_cost_curve.cc
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

#include <cstdlib>
#include <array>

struct PingMsg : vt::Message {
  PingMsg() = default;
  explicit PingMsg(int64_t size) {
    payload_.resize(size);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | payload_;
  }

  std::vector<char> payload_;
};

static constexpr int64_t const max_bytes = 0x1000000;
static int pings = 64;
static bool is_done = false;

static void done(PingMsg* msg) {
  is_done = true;
}

static void handler(PingMsg*) {
  static int count = 0;
  count++;
  if (count == pings) {
    auto msg = vt::makeMessage<PingMsg>(1);
    vt::theMsg()->sendMsgAuto<PingMsg,done>(0, msg.get());
    count = 0;
  }
}

template <int64_t bytes>
void sender() {
  auto start = vt::timing::Timing::getCurrentTime();
  for (int i = 0; i < pings; i++) {
    auto msg = vt::makeMessage<PingMsg>(bytes);
    vt::theMsg()->sendMsgAuto<PingMsg,handler>(1, msg.get());
  }
  while (not is_done) vt::runScheduler();
  is_done = false;
  auto time = (vt::timing::Timing::getCurrentTime() - start) / pings;
  auto Mb = static_cast<double>(bytes) / 1024.0 / 1024.0;
  fmt::print("{:<8} {:<16} 0x{:<10x} {:<22} {:<22}\n", pings, bytes, bytes, Mb, time);
}

template <int64_t bytes>
void send() {
  sender<bytes>();
  send<bytes << 1>();
}

template <>
void send<max_bytes>() {
  sender<max_bytes>();
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  auto const this_node = vt::theContext()->getNode();

  if (argc == 2) {
    pings = atoi(argv[1]);
  } else {
    pings = 10;
  }

  if (this_node == 0) {
    fmt::print(
      "{:<8} {:<16} 0x{:<10} {:<22} {:<22}\n",
      "Pings", "Bytes", "Bytes", "Mb", "Time per"
    );
    send<0x1>();
  }

  vt::finalize();
}
