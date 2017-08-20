
#if ! defined __RUNTIME_TRANSPORT_RDMA_MSG__
#define __RUNTIME_TRANSPORT_RDMA_MSG__

#include "common.h"
#include "message.h"
#include "rdma_common.h"

namespace runtime { namespace rdma {

template <typename EnvelopeT>
struct RequestDataMessage : ActiveMessage<EnvelopeT> {

  RequestDataMessage(
    rdma_op_t const& in_op, node_t const in_node, rdma_handle_t const& in_han,
    byte_t const& in_num_bytes = no_byte, byte_t const& in_offset = no_byte
  ) : ActiveMessage<EnvelopeT>(),
      op_id(in_op), requesting(in_node), rdma_handle(in_han),
      num_bytes(in_num_bytes), offset(in_offset)
  { }

  rdma_op_t op_id = no_rdma_op;
  node_t requesting = uninitialized_destination;
  rdma_handle_t rdma_handle = no_rdma_handle;
  byte_t num_bytes = no_byte;
  byte_t offset = no_byte;
  bool is_user_msg = false;
};

template <typename EnvelopeT>
struct SendDataMessage : ActiveMessage<EnvelopeT> {
  SendDataMessage(
    rdma_op_t const& in_op, byte_t const& in_num_bytes, byte_t const& in_offset,
    tag_t const& in_mpi_tag, rdma_handle_t const& in_han = no_rdma_handle,
    node_t const& back = uninitialized_destination
  ) : ActiveMessage<EnvelopeT>(),
    op_id(in_op), num_bytes(in_num_bytes), rdma_handle(in_han),
    mpi_tag_to_recv(in_mpi_tag), send_back(back), offset(in_offset)
  { }

  rdma_handle_t rdma_handle = no_rdma_handle;
  node_t send_back = uninitialized_destination;
  tag_t mpi_tag_to_recv = no_tag;
  rdma_op_t op_id = no_rdma_op;
  byte_t num_bytes = no_byte;
  byte_t offset = no_byte;
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
    rdma_type_t const& in_type, rdma_handle_t const& in_han,
    tag_t const& in_channel_tag, node_t const& in_target,
    node_t const& in_non_target
  ) : CallbackMessage(), type(in_type), rdma_handle(in_han),
      target(in_target), non_target(in_non_target),
      channel_tag(in_channel_tag)
  { }

  bool has_bytes = false;
  tag_t channel_tag = no_tag;
  rdma_handle_t rdma_handle = no_rdma_handle;
  rdma_type_t type = uninitialized_rdma_type;
  node_t target = uninitialized_destination;
  node_t non_target = uninitialized_destination;
};

struct GetInfoChannel : runtime::CallbackMessage {
  GetInfoChannel(byte_t const& in_num_bytes)
    : CallbackMessage(), num_bytes(in_num_bytes)
  { }

  byte_t num_bytes = no_byte;
};

struct ChannelMessage : runtime::CallbackMessage {
  using rdma_type_t = Type;

  ChannelMessage(
    rdma_type_t const& in_type, rdma_handle_t const& in_han,
    byte_t const& in_num_bytes, tag_t const& in_channel_tag
  ) : CallbackMessage(), type(in_type), han(in_han), num_bytes(in_num_bytes),
      channel_tag(in_channel_tag)
  { }

  tag_t channel_tag = no_tag;
  rdma_type_t type = uninitialized_rdma_type;
  rdma_handle_t han = no_rdma_handle;
  byte_t num_bytes = no_byte;
};

using GetMessage = RequestDataMessage<EpochTagEnvelope>;
using GetBackMessage = SendDataMessage<EpochTagEnvelope>;

using PutMessage = SendDataMessage<EpochTagEnvelope>;
using PutBackMessage = RDMAOpFinishedMessage<EpochTagEnvelope>;

using DestroyChannel = ChannelMessage;

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_COMMON__*/
