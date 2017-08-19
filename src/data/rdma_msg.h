
#if ! defined __RUNTIME_TRANSPORT_RDMA_MSG__
#define __RUNTIME_TRANSPORT_RDMA_MSG__

#include "common.h"
#include "message.h"
#include "rdma_common.h"

namespace runtime { namespace rdma {

template <typename EnvelopeT>
struct GetActiveMessage : ActiveMessage<EnvelopeT> {

  GetActiveMessage(
    rdma_op_t const& in_op, node_t const in_node, rdma_handle_t const& in_han,
    byte_t const& in_num_bytes = no_byte
  ) : ActiveMessage<EnvelopeT>(),
      op_id(in_op), requesting(in_node), rdma_handle(in_han),
      num_bytes(in_num_bytes)
  { }

  rdma_op_t op_id;
  node_t requesting = uninitialized_destination;
  rdma_handle_t rdma_handle = no_rdma_handle;
  byte_t num_bytes = no_byte;
  bool is_user_msg = false;
};

using GetMessage = GetActiveMessage<EpochTagEnvelope>;
//using PutMessage = GetActiveMessage<EpochTagEnvelope>;

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_COMMON__*/
