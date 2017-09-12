
#if ! defined __RUNTIME_TRANSPORT_RDMA_MSG__
#define __RUNTIME_TRANSPORT_RDMA_MSG__

#include "common.h"
#include "message.h"
#include "rdma_common.h"

namespace runtime { namespace rdma {

template <typename EnvelopeT>
struct RequestDataMessage : ActiveMessage<EnvelopeT> {

  RequestDataMessage(
    rdma_op_t const& in_op, NodeType const in_node, RDMA_HandleType const& in_han,
    ByteType const& in_num_bytes = no_byte, ByteType const& in_offset = no_byte
  ) : ActiveMessage<EnvelopeT>(),
      op_id(in_op), requesting(in_node), rdma_handle(in_han),
      num_bytes(in_num_bytes), offset(in_offset)
  { }

  rdma_op_t op_id = no_rdma_op;
  NodeType requesting = uninitialized_destination;
  RDMA_HandleType rdma_handle = no_rdma_handle;
  ByteType num_bytes = no_byte;
  ByteType offset = no_byte;
  bool is_user_msg = false;
};

template <typename EnvelopeT>
struct SendDataMessage : ActiveMessage<EnvelopeT> {
  SendDataMessage(
    rdma_op_t const& in_op, ByteType const& in_num_bytes, ByteType const& in_offset,
    TagType const& in_mpi_tag, RDMA_HandleType const& in_han = no_rdma_handle,
    NodeType const& back = uninitialized_destination,
    NodeType const& in_recv_node = uninitialized_destination
  ) : ActiveMessage<EnvelopeT>(),
    op_id(in_op), num_bytes(in_num_bytes), rdma_handle(in_han),
    mpi_tag_to_recv(in_mpi_tag), send_back(back), offset(in_offset),
    recv_node(in_recv_node)
  { }

  RDMA_HandleType rdma_handle = no_rdma_handle;
  NodeType send_back = uninitialized_destination;
  NodeType recv_node = uninitialized_destination;
  TagType mpi_tag_to_recv = no_tag;
  rdma_op_t op_id = no_rdma_op;
  ByteType num_bytes = no_byte;
  ByteType offset = no_byte;
};

template <typename EnvelopeT>
struct RDMAOpFinishedMessage : ActiveMessage<EnvelopeT> {
  RDMAOpFinishedMessage(rdma_op_t const& in_op)
    : ActiveMessage<EnvelopeT>(), op_id(in_op)
  { }

  rdma_op_t op_id = no_rdma_op;
};

struct CreateChannel : runtime::CallbackMessage {
  using rdma_type_t = Type;

  CreateChannel(
    rdma_type_t const& in_type, RDMA_HandleType const& in_han,
    TagType const& in_channel_tag, NodeType const& in_target,
    NodeType const& in_non_target
  ) : CallbackMessage(), type(in_type), rdma_handle(in_han),
      target(in_target), non_target(in_non_target),
      channel_tag(in_channel_tag)
  { }

  bool has_bytes = false;
  TagType channel_tag = no_tag;
  RDMA_HandleType rdma_handle = no_rdma_handle;
  rdma_type_t type = uninitialized_rdma_type;
  NodeType target = uninitialized_destination;
  NodeType non_target = uninitialized_destination;
};

struct GetInfoChannel : runtime::CallbackMessage {
  GetInfoChannel(ByteType const& in_num_bytes)
    : CallbackMessage(), num_bytes(in_num_bytes)
  { }

  ByteType num_bytes = no_byte;
};

struct ChannelMessage : runtime::CallbackMessage {
  using rdma_type_t = Type;

  ChannelMessage(
    rdma_type_t const& in_type, RDMA_HandleType const& in_han,
    ByteType const& in_num_bytes, TagType const& in_channel_tag,
    NodeType const& in_non_target = uninitialized_destination,
    NodeType const& in_override_target = uninitialized_destination
  ) : CallbackMessage(), type(in_type), han(in_han), num_bytes(in_num_bytes),
      channel_tag(in_channel_tag), non_target(in_non_target),
      override_target(in_override_target)
  { }

  TagType channel_tag = no_tag;
  rdma_type_t type = uninitialized_rdma_type;
  RDMA_HandleType han = no_rdma_handle;
  ByteType num_bytes = no_byte;
  NodeType non_target = uninitialized_destination;
  NodeType override_target = uninitialized_destination;
};

using GetMessage = RequestDataMessage<EpochTagEnvelope>;
using GetBackMessage = SendDataMessage<EpochTagEnvelope>;

using PutMessage = SendDataMessage<EpochTagEnvelope>;
using PutBackMessage = RDMAOpFinishedMessage<EpochTagEnvelope>;

using DestroyChannel = ChannelMessage;

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_COMMON__*/
