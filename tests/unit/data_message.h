/*
//@HEADER
// *****************************************************************************
//
//                                data_message.h
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

#if ! defined INCLUDED_UNIT_DATA_MESSAGE_H
#define INCLUDED_UNIT_DATA_MESSAGE_H

#include "vt/messaging/message.h"

#include <array>
#include <cstdint>

namespace vt { namespace tests { namespace unit {

using NumBytesType = int64_t;
using ByteType = char;

template <typename MessageT, NumBytesType num_bytes>
struct TestStaticBytesMsg : MessageT {
  using MessageParentType = MessageT;

  // This type is fundamentally broken as a message in that
  // it does not support it's own serialization as required.
  // It is not even valid to set the serialize mode here
  // or it will fail to align with subtype expectations.
  // (It will be sent as byte-copyable since there is no serializer.)

  static_assert(
    std::is_base_of<BaseMessage, MessageT>::value,
    "Must derive from Message."
  );

  TestStaticBytesMsg() : MessageT() { }

  explicit TestStaticBytesMsg(NumBytesType const& in_bytes)
    : MessageT(), bytes(in_bytes)
  { }

  explicit TestStaticBytesMsg(
    NumBytesType const& in_bytes, std::array<ByteType, num_bytes>&& arr
  ) : MessageT(), payload(std::forward<std::array<ByteType, num_bytes>>(arr)),
      bytes(in_bytes)
  { }

  std::array<ByteType, num_bytes> payload{};
  NumBytesType bytes = 0;
};

template <typename MessageT, NumBytesType num_bytes>
struct TestStaticSerialBytesMsg : TestStaticBytesMsg<MessageT,num_bytes> {
  using MessageParentType = TestStaticBytesMsg<MessageT,num_bytes>;
  vt_msg_serialize_required();

  // Derived type serializes parent type's members.. (don't do this)
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageT::serialize(s);
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

#endif /* INCLUDED_UNIT_DATA_MESSAGE_H*/
