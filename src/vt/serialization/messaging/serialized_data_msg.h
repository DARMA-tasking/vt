/*
//@HEADER
// *****************************************************************************
//
//                            serialized_data_msg.h
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

#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_DATA_MSG_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_DATA_MSG_H

#include "vt/config.h"
#include "vt/messaging/message.h"

namespace vt { namespace serialization {

using SizeType = std::size_t;

static constexpr SizeType const serialized_msg_eager_size = 128;

struct NoneVrt { };

template <typename T, typename MessageT>
struct SerializedDataMsgAny : MessageT {
  SerializedDataMsgAny() = default;

  ByteType ptr_size = 0;
  HandlerType handler = uninitialized_handler;
  TagType data_recv_tag = no_tag;
  NodeType from_node = uninitialized_destination;
};

template <typename T>
using SerializedDataMsg = SerializedDataMsgAny<T, Message>;

using NumBytesType = int64_t;

template <typename UserMsgT, typename MessageT, NumBytesType num_bytes>
struct SerialPayloadMsg : MessageT {
  std::array<SerialByteType, num_bytes> payload{};
  NumBytesType bytes = 0;

  SerialPayloadMsg() : MessageT() { }

  explicit SerialPayloadMsg(NumBytesType const& in_bytes)
    : MessageT(), bytes(in_bytes)
  { }

  explicit SerialPayloadMsg(
    NumBytesType const& in_bytes, std::array<ByteType, num_bytes>&& arr
  ) : MessageT(), payload(std::forward<std::array<ByteType, num_bytes>>(arr)),
      bytes(in_bytes)
  { }
};

template <typename UserMsgT, typename BaseEagerMsgT>
using SerialEagerPayloadMsg = SerialPayloadMsg<
  UserMsgT, SerializedDataMsgAny<UserMsgT, BaseEagerMsgT>,
  serialized_msg_eager_size
>;

}} /* end namespace vt::serialization */

#endif /*INCLUDED_SERIALIZATION_MESSAGING/SERIALIZED_DATA_MSG_H*/
