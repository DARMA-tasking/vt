/*
//@HEADER
// *****************************************************************************
//
//                                  rdma_msg.h
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

#if !defined INCLUDED_RDMA_RDMA_MSG_H
#define INCLUDED_RDMA_RDMA_MSG_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/rdma/rdma_common.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"

namespace vt { namespace rdma {

template <typename EnvelopeT>
struct RequestDataMessage : ActiveMessage<EnvelopeT> {

  RequestDataMessage(
    RDMA_OpType const& in_op, NodeType const in_node,
    RDMA_HandleType const& in_han, ByteType const& in_num_bytes = no_byte,
    ByteType const& in_offset = no_byte
  ) : ActiveMessage<EnvelopeT>(),
      op_id(in_op), requesting(in_node), rdma_handle(in_han),
      num_bytes(in_num_bytes), offset(in_offset)
  { }

  RDMA_OpType op_id = no_rdma_op;
  NodeType requesting = uninitialized_destination;
  RDMA_HandleType rdma_handle = no_rdma_handle;
  ByteType num_bytes = no_byte;
  ByteType offset = no_byte;
  bool is_user_msg = false;
};

template <typename EnvelopeT>
struct SendDataMessage : ActiveMessage<EnvelopeT> {
  SendDataMessage() = default;
  SendDataMessage(
    RDMA_OpType const& in_op, ByteType const& in_num_bytes,
    ByteType const& in_offset, TagType const& in_mpi_tag,
    RDMA_HandleType const& in_han = no_rdma_handle,
    NodeType const& back = uninitialized_destination,
    NodeType const& in_recv_node = uninitialized_destination,
    bool const in_packed_direct = false
  ) : rdma_handle(in_han), send_back(back),
      recv_node(in_recv_node), mpi_tag_to_recv(in_mpi_tag), op_id(in_op),
      num_bytes(in_num_bytes), offset(in_offset),
      packed_direct(in_packed_direct)
  { }

  int nchunks = 0;
  RDMA_HandleType rdma_handle = no_rdma_handle;
  NodeType send_back = uninitialized_destination;
  NodeType recv_node = uninitialized_destination;
  TagType mpi_tag_to_recv = no_tag;
  RDMA_OpType op_id = no_rdma_op;
  ByteType num_bytes = no_byte;
  ByteType offset = no_byte;
  bool packed_direct = false;
};

template <typename EnvelopeT>
struct RDMAOpFinishedMessage : ActiveMessage<EnvelopeT> {
  explicit RDMAOpFinishedMessage(RDMA_OpType const& in_op)
    : ActiveMessage<EnvelopeT>(), op_id(in_op)
  { }

  RDMA_OpType op_id = no_rdma_op;
};

struct GetInfoChannel : vt::Message {
  explicit GetInfoChannel(ByteType in_num_bytes)
    : num_bytes(in_num_bytes)
  { }

  ByteType num_bytes = no_byte;
};

struct CreateChannel : vt::Message {
  using RDMA_TypeType = Type;

  CreateChannel(
    RDMA_TypeType const& in_type, RDMA_HandleType const& in_han,
    TagType const& in_channel_tag, NodeType const& in_target,
    NodeType const& in_non_target, Callback<GetInfoChannel> in_cb
  ) : channel_tag(in_channel_tag), rdma_handle(in_han),
      type(in_type), target(in_target), non_target(in_non_target), cb(in_cb)
  { }

  bool has_bytes = false;
  TagType channel_tag = no_tag;
  RDMA_HandleType rdma_handle = no_rdma_handle;
  RDMA_TypeType type = uninitialized_rdma_type;
  NodeType target = uninitialized_destination;
  NodeType non_target = uninitialized_destination;
  Callback<GetInfoChannel> cb;
};

struct ChannelMessage : vt::Message {
  using RDMA_TypeType = Type;

  ChannelMessage(
    RDMA_TypeType const& in_type, RDMA_HandleType const& in_han,
    ByteType const& in_num_bytes, TagType const& in_channel_tag,
    Callback<> in_cb,
    NodeType const& in_non_target = uninitialized_destination,
    NodeType const& in_override_target = uninitialized_destination
  ) : channel_tag(in_channel_tag), type(in_type),
      han(in_han), num_bytes(in_num_bytes), non_target(in_non_target),
      override_target(in_override_target), cb(in_cb)
  { }

  TagType channel_tag = no_tag;
  RDMA_TypeType type = uninitialized_rdma_type;
  RDMA_HandleType han = no_rdma_handle;
  ByteType num_bytes = no_byte;
  NodeType non_target = uninitialized_destination;
  NodeType override_target = uninitialized_destination;
  Callback<> cb;
};

using GetMessage = RequestDataMessage<EpochTagEnvelope>;
using GetBackMessage = SendDataMessage<EpochTagEnvelope>;

using PutMessage = SendDataMessage<EpochTagEnvelope>;
using PutBackMessage = RDMAOpFinishedMessage<EpochTagEnvelope>;

using DestroyChannel = ChannelMessage;

template <typename StateT>
struct StateMessage : vt::Message {
  StateT* const state = nullptr;

  explicit StateMessage(StateT* const in_state)
    : state(in_state)
  { }
};

}} //end namespace vt::rdma

#endif /*INCLUDED_RDMA_RDMA_MSG_H*/
