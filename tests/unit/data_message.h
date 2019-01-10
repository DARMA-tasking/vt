/*
//@HEADER
// ************************************************************************
//
//                          data_message.h
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

#if ! defined __VIRTUAL_TRANSPORT_DATA_MESSAGE__
#define __VIRTUAL_TRANSPORT_DATA_MESSAGE__

#include "vt/transport.h"

#include <array>
#include <cstdint>

namespace vt { namespace tests { namespace unit {

using NumBytesType = int64_t;
using ByteType = char;

template <typename MessageT, NumBytesType num_bytes>
struct TestStaticBytesMsg : MessageT {
  std::array<ByteType, num_bytes> payload{};
  NumBytesType bytes = 0;

  TestStaticBytesMsg() : MessageT() { }

  explicit TestStaticBytesMsg(NumBytesType const& in_bytes)
    : MessageT(), bytes(in_bytes)
  { }

  explicit TestStaticBytesMsg(
    NumBytesType const& in_bytes, std::array<ByteType, num_bytes>&& arr
  ) : MessageT(), payload(std::forward<std::array<ByteType, num_bytes>>(arr)),
      bytes(in_bytes)
  { }
};

template <typename MessageT, NumBytesType num_bytes>
struct TestStaticSerialBytesMsg : TestStaticBytesMsg<MessageT,num_bytes> {
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageT::serializeThis(s);
    s | TestStaticBytesMsg<MessageT,num_bytes>::bytes;
    for (auto&& elm : TestStaticBytesMsg<MessageT,num_bytes>::payload) {
      s | elm;
    }
  }
};

template <NumBytesType num_bytes>
using TestStaticBytesNormalMsg = TestStaticBytesMsg<vt::Message, num_bytes>;

template <NumBytesType num_bytes>
using TestStaticBytesShortMsg = TestStaticBytesMsg<vt::ShortMessage, num_bytes>;

template <NumBytesType num_bytes>
using TestStaticBytesSerialMsg = TestStaticSerialBytesMsg<vt::Message, num_bytes>;

template <typename MessageT, typename T, int len>
struct WaitInfoMsg : MessageT {
  T info[len];

  explicit WaitInfoMsg(T in_info[len]) : MessageT() {
    for (int i = 0; i < len; i++) {
      info[i] = in_info[i];
    }
  }
};

}}} // end namespace vt::tests::unit

#endif /* __VIRTUAL_TRANSPORT_DATA_MESSAGE__*/
